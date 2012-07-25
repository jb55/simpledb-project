/*
 * SimpleDB.c - A simple database which stores specific data efficiently.
 * Created by: Bill Casarin
 * Email: billcasarin@gmail.com
 * Date: Sept 11, 2008
 */

#include "orphanlist.h"
#include "simpledb.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_ORPHAN_ALLOC 524288000
#define MAX_RECORDS 40000

/* create sdb context, init files if they don't exist */
int sdb_init(char* db_name, struct sdb_context** context_out)
{
    FILE* file;
    char* filename;
    struct sdb_context* context;

    *context_out = (struct sdb_context*) malloc(sizeof(struct sdb_context));
    context = *context_out;

    /* 
     * Set database name, this will be used for the semaphore id 
     * and for the filename
     */
    context->db_name = (char*) malloc(strlen(db_name)+1);
    strcpy(context->db_name, db_name);

    /* create orphan list */
    olist_create(&(context->orphan_list));

    /* generate the filename */
    filename = (char*) malloc(strlen(db_name)+5);
    strcpy(filename, db_name);
    strcat(filename, ".sdb");

    /* store the filename in the context */
    context->db_filename = (char*) malloc(strlen(filename)+1);
    strcpy(context->db_filename, filename);

    /* create the file on disk if it doesn't already exist */
    file = fopen(filename, "a");
    fclose(file);

    /* store index filename in the context */
    strcpy(filename, db_name);
    strcat(filename, ".sdi");
    context->db_index_filename = (char*) malloc(strlen(filename)+1);
    strcpy(context->db_index_filename, filename);

    /* create the index file */
    file = fopen(filename, "a");
    fclose(file);

    /* store the orphan filename in the context */
    strcpy(filename, db_name);
    strcat(filename, ".sdo");
    context->db_orphan_filename = (char*) malloc(strlen(filename)+1);
    strcpy(context->db_orphan_filename, filename);

    file = fopen(filename, "r");

    if( !file )
        file = fopen(filename, "w");
    else
        _sdb_load_orphans(context);

    fclose(file);

    /* create the semaphore */
    semaphore_create(context->db_filename, &(context->sem));

    free(filename);
    return SDB_OK;
}

int sdb_close(struct sdb_context* context)
{
    free(context->db_name);
    free(context->db_filename);
    free(context->db_index_filename);
    free(context->db_orphan_filename);
    olist_destroy(context->orphan_list);
    semaphore_destroy(context->sem);
    free(context);
    
    return SDB_OK;
}


int sdb_insert(struct sdb_context* context, struct sdb_record* record)
{
    int num_records, origin;
    int res;
    unsigned int uid;
    unsigned int offset;
    int r_size;
    struct sdb_orphan orphan;

    if( !context )
        return SDB_NOT_INITIALIZED;

    if( (res = sdb_get_record_count(context, &num_records)) != SDB_OK )
        return res;

    uid = num_records + 1;

    if( uid > MAX_RECORDS )
        return SDB_MAX_RECORDS;

    if( semaphore_lock(context->sem) == SEM_LOCKED )
        return SDB_RESOURCE_LOCKED;

    /* write the new record count */
    if( (res = _sdb_write_record_count(context, uid)) != SDB_OK )
    {
        semaphore_unlock(context->sem);
        return res;
    }
    r_size = _sdb_calc_record_size(record);

    /* see if there's any orphan spots available */
    if( (res = _sdb_find_orphan_candidate(context, r_size, &orphan)) != SDB_OK )
    {
        origin = SEEK_END;
        offset = 0;
    }
    else
    {
        origin = SEEK_SET;
        offset = orphan.offset;
    }

    if( (res = _sdb_write_record(context, uid, offset, origin, record, &offset)) != SDB_OK )
    {
        semaphore_unlock(context->sem);
        return res;
    }

    /* now update the index */
    if( (res = _sdb_update_index(context, uid, offset)) != SDB_OK )
    {
        semaphore_unlock(context->sem);
        return res;
    }

    semaphore_unlock(context->sem);

    return SDB_OK;
}

/* allocate a record */
void sdb_create_record(char* first_name, 
    char* last_name, time_t date, struct sdb_record** record)
{
    (*record) = (struct sdb_record*) malloc(sizeof(struct sdb_record));
    (*record)->first_name = (char*) malloc(strlen(first_name)+1);
    strcpy((*record)->first_name, first_name);
    (*record)->last_name = (char*) malloc(strlen(last_name)+1);
    strcpy((*record)->last_name, last_name);
    (*record)->date = date;
}

/* deallocate a record */
void sdb_destroy_record(struct sdb_record* record)
{
    if( !record )
        return;

    free(record->first_name);
    free(record->last_name);
    free(record);
    record = NULL;

    return;
}


/* points the uid at a given offset */
int _sdb_update_index(struct sdb_context* context, 
    unsigned int uid, unsigned int offset)
{
    int end;
    int write_offset;

    FILE* file = fopen(context->db_index_filename, "rb+");
    if( !file )
        return SDB_COULD_NOT_OPEN_FILE;

    fseek(file, 0, SEEK_END);
    end = ftell(file);
    
    /* 
     * Make sure we're not trying to update an
     * index that is greater then the last uid
     */
    write_offset = sizeof(uid)*(uid-1);
    if( write_offset > end )
        return SDB_INDEX_UPDATE_ERROR;    
    
    fseek(file, write_offset, SEEK_SET);
    fwrite(&offset, sizeof(offset), 1, file);
    fclose(file);

    return SDB_OK;
}

/* calculate the size of a record (doesn't include the header) */
int _sdb_calc_record_size(struct sdb_record* record)
{
    int len = 0;

    len += strlen(record->first_name)+1;
    len += strlen(record->last_name)+1;
    len += sizeof(record->date);

    return len;
}

/* write the orphan list to disk for persistence */
int _sdb_write_orphans(struct sdb_context* context)
{
    FILE *file;
    onode* c_node;
    int count;

    file = fopen(context->db_orphan_filename, "wb");
    if( !file )
        return SDB_COULD_NOT_OPEN_FILE;

    count = context->orphan_list->node_count;
    fwrite(&count, sizeof(count), 1, file);

    c_node = context->orphan_list->head;
    for(; c_node != NULL; c_node = c_node->next)
        fwrite(&(c_node->orphan), sizeof(c_node->orphan), 1, file);

    fclose(file);
    return SDB_OK;
}


/* load known orphans from dbname.sdo into memory */
void _sdb_load_orphans(struct sdb_context* context)
{ 
    int num_orphans = 0, alloc_size;
    int i = 0;
    FILE* file;
    struct sdb_orphan* orphans;

    file = fopen(context->db_orphan_filename, "r");
    if( !file )
        return;

    /* get the orphan count from the file */
    fread(&num_orphans, sizeof(num_orphans), 1, file);

    if( num_orphans == 0 || num_orphans == -1 )
        return;
    alloc_size = sizeof(struct sdb_orphan)*num_orphans;

    if( alloc_size > MAX_ORPHAN_ALLOC )
        return;
    orphans = (struct sdb_orphan*) malloc(alloc_size);

    /* read the orphans from the file */
    fread(orphans, sizeof(struct sdb_orphan), num_orphans, file);
    
    /* add orphans to linked list */
    for(i = 0; i < num_orphans; ++i)
        olist_insert(context->orphan_list, &(orphans[i]));

    free(orphans);
    fclose(file);
}


/* write the number of records to the database header */
int _sdb_write_record_count(struct sdb_context* context, int count)
{
    FILE* file;
    
    if( !context )
        return SDB_NOT_INITIALIZED;

    file = fopen(context->db_filename, "rb+");
    if( !file )
        return SDB_COULD_NOT_OPEN_FILE;

    fseek(file, 0, SEEK_SET);
    fwrite(&count, sizeof(count), 1, file);
    fclose(file);

    return SDB_OK;
}

/* gets the number of records in the database */
int sdb_get_record_count(struct sdb_context* context, int *count_out)
{
    FILE* file;
    int num_records = 0;

    if( !context )
        return SDB_NOT_INITIALIZED;

    if( semaphore_lock(context->sem) == SEM_LOCKED )
        return SDB_RESOURCE_LOCKED;

    file = fopen(context->db_filename, "rb");

    if( !file )
    {
        semaphore_unlock(context->sem);
        return SDB_COULD_NOT_OPEN_FILE;
    }
    
    fread(&num_records, sizeof(num_records), 1, file);
    if( feof(file) )
        num_records = 0;
    fclose(file);

    semaphore_unlock(context->sem);
    *count_out = num_records;

    return SDB_OK;
}


/* 
 * Updates a record in the database. If the record is larger
 * then the original, a new record is added onto the end
 * of the database and the index is updated to the new record.
 * The orphaned record is added to the orphan list,
 * to be reused by new or updated records.
 */
int sdb_update(struct sdb_context* context, 
    unsigned int uid, struct sdb_record* record)
{
    int res;
    struct sdb_record* db_record;
    int update_size;
    int db_size;
    unsigned int db_offset;
    unsigned int offset;
    struct sdb_orphan orphan;

    if( (res = sdb_find(context, uid, &db_record, &db_offset)) != SDB_OK )
        return res;

    if( semaphore_lock(context->sem) == SEM_LOCKED )
        return SDB_RESOURCE_LOCKED;

    update_size = _sdb_calc_record_size(record);
    db_size = _sdb_calc_record_size(db_record);

    /* 
     * If the new record is bigger then the old one, append to end
     * and orphan the old record
     */
    if( update_size > db_size )
    {
        /* attempt to match an orphan */
        res = _sdb_find_orphan_candidate(context, update_size, &orphan);
        if( res == SDB_NO_ORPHANS_MATCH )
            _sdb_write_record(context, uid, 0, SEEK_END, record, &offset);
        else
            _sdb_write_record(context, uid, orphan.offset, SEEK_SET, record, &offset);

        _sdb_update_index(context, uid, offset);
    
        if( orphan.offset != db_offset )
        {
            orphan.size = db_size;
            orphan.offset = db_offset;
            olist_insert(context->orphan_list, &orphan);
            _sdb_write_orphans(context);
        }
    }
    else
    {
        _sdb_write_record(context, uid, db_offset, SEEK_SET, record, &offset);
        assert( offset == db_offset );
    }

    semaphore_unlock(context->sem);

    /* cleanup from sdb_find */
    sdb_destroy_record(db_record);
    return SDB_OK;        
}

int sdb_find(struct sdb_context* context, 
    unsigned int uid, struct sdb_record** record_out, 
    unsigned int* offset_out)
{
    unsigned int record_offset;
    int rec_count, res;

    if( !context )
        return SDB_NOT_INITIALIZED;

    if( (res = sdb_get_record_count(context, &rec_count)) != SDB_OK )
        return res;

    if( semaphore_lock(context->sem) == SEM_LOCKED )
        return SDB_RESOURCE_LOCKED;

    if( uid > rec_count )
    {
        semaphore_unlock(context->sem);
        return SDB_RECORD_NOT_FOUND;
    }

    _sdb_get_record_offset(context, uid, &record_offset);
    _sdb_load_record(context, record_offset, record_out);

    semaphore_unlock(context->sem);

    *offset_out = record_offset;
    return SDB_OK;
}


int _sdb_get_record_offset(struct sdb_context* context,
    unsigned int uid, unsigned int* offset_out)
{
    FILE* file;
    int offset;

    file = fopen(context->db_index_filename, "rb");
    if( !file )
        return SDB_COULD_NOT_OPEN_FILE;

    offset = sizeof(uid)*(uid-1);
    fseek(file, offset, SEEK_SET);
    fread(offset_out, sizeof(uid), 1, file);

    fclose(file);
    return SDB_OK;
}

int _sdb_write_record(struct sdb_context* context,
    unsigned int uid, unsigned int offset, int origin, 
    struct sdb_record* record, unsigned int* offset_out)
{
    FILE* file;
    int r_size;

    file = fopen(context->db_filename, "rb+");
    if( !file )
        return SDB_COULD_NOT_OPEN_FILE;

    r_size = _sdb_calc_record_size(record);

    fseek(file, offset, origin);
    *offset_out = ftell(file);

    /* write the record header (uid, size) */
    fwrite(&uid, sizeof(uid), 1, file);
    fwrite(&r_size, sizeof(r_size), 1, file);

    /* write the actual record */
    fwrite(record->first_name, strlen(record->first_name)+1, 1, file);
    fwrite(record->last_name, strlen(record->last_name)+1, 1, file);
    fwrite(&(record->date), sizeof(record->date), 1, file);

    fclose(file);

    return SDB_OK;
}

/* loads a specific record from the database */
int _sdb_load_record(struct sdb_context* context, 
    int offset, struct sdb_record** record_out)
{
    int c, i, count[2], first_offset;
    char* names[2]; 
    time_t date;
    FILE* file;

    file = fopen(context->db_filename, "rb");
    if( !file )
        return SDB_COULD_NOT_OPEN_FILE;

    first_offset = offset+(sizeof(int)*2);
    fseek(file, first_offset, SEEK_SET);

    /* get string lengths */
    for(i = 0; i < 2; ++i)
    {
        count[i] = 0;
        while( (c=fgetc(file)) != '\0' )
            count[i]++;
        count[i]++;

        names[i] = (char*) malloc(count[i]);
    }

    fseek(file, first_offset, SEEK_SET);
    fread(names[0], count[0], 1, file);
    fread(names[1], count[1], 1, file);

    /* read date */
    fread(&date, sizeof(date), 1, file);

    #define ro (*record_out)
    ro = (struct sdb_record*) malloc(sizeof(struct sdb_record));
    ro->first_name = names[0];
    ro->last_name = names[1];
    ro->date = date;
    #undef ro

    fclose(file);
    return SDB_OK;
}


/* looks for an orphan candidate which matches the requested size */
int _sdb_find_orphan_candidate(struct sdb_context* context,
    int size, struct sdb_orphan *orphan_out)
{
    onode* c_node = context->orphan_list->head;
    for(; c_node != NULL; c_node = c_node->next)
    {
        if( size <= c_node->orphan.size )
        {
            *orphan_out = c_node->orphan;
            olist_remove(context->orphan_list, c_node);
            _sdb_write_orphans(context);
            return SDB_OK;
        }
    }
    return SDB_NO_ORPHANS_MATCH;
}








