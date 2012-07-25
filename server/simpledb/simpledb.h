#ifndef _H_SIMPLEDB_
#define _H_SIMPLEDB_

#include <time.h>
#include <stdio.h>
#include "semaphore.h"

/* reserve 128 orphan spots in memory initially */
#define INIT_ORPHAN_SIZE 128
#define ORPHAN_EXPAND 1.5

struct sdb_orphan
{
    unsigned int offset;
    unsigned int size;
};

struct sdb_context
{
    char* db_name;
    char* db_filename;
    char* db_index_filename;
    char* db_orphan_filename;
    struct orphan_list* orphan_list; 
    semaphore sem;
};

struct sdb_record
{
    char* first_name;
    char* last_name;
    time_t date;
};

enum e_sdb_result
{
    SDB_OK,
    SDB_RESOURCE_LOCKED,
    SDB_RECORD_NOT_FOUND,
    SDB_MAX_RECORDS,
    SDB_NOT_INITIALIZED,
    SDB_COULD_NOT_OPEN_FILE,
    SDB_INDEX_UPDATE_ERROR,
    SDB_NO_ORPHANS_MATCH
};

/* main functions */
int sdb_init(char* db_name, struct sdb_context** context_out);
int sdb_close(struct sdb_context* context);
int sdb_update(struct sdb_context* context, unsigned int uid, struct sdb_record* record);
int sdb_insert(struct sdb_context* context,struct sdb_record* record);
int sdb_find(struct sdb_context* context,unsigned int uid, struct sdb_record** record_out, unsigned int* offset_out);
void sdb_create_record(char* first_name, char* last_name, time_t date, struct sdb_record** record);
void sdb_destroy_record(struct sdb_record* record);
int sdb_get_record_count(struct sdb_context* context, int *count_out);

/* 
 * internal use only 
 * not thread safe
 */
void _sdb_load_orphans(struct sdb_context* context);
int _sdb_write_orphans(struct sdb_context* context);
int _sdb_load_record(struct sdb_context* context, 
    int offset, struct sdb_record** record_out);
int _sdb_write_record(struct sdb_context* context, 
    unsigned int uid, unsigned int offset, int origin, 
    struct sdb_record* record, unsigned int* offset_out);
int _sdb_write_record_count(struct sdb_context* context, int count);
int _sdb_calc_record_size(struct sdb_record* record);
int _sdb_update_index(struct sdb_context* context, 
    unsigned int uid, unsigned int offset);
int _sdb_get_record_offset(struct sdb_context* context,
    unsigned int uid, unsigned int* offset_out);
int _sdb_find_orphan_candidate(struct sdb_context* context,
    int size, struct sdb_orphan *orphan_out);

#endif
