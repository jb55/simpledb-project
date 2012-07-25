#include "../simpledb.h"
#include <time.h>
#include <stdio.h>

struct sdb_record records[] =
{
    {"Bill", "Casarin", 1},
    {"This is a test", "OOk then", 234234},
    {"This is a very long first name lol", "LASTnAMELASTNAME", 1232343245},
    {"Firstname", "Lastname", 0},
    {"JOHN", "JOHNSLASTNAME", 123123}
};

int main(void)
{
    int res, i;
    unsigned int offset;
    struct sdb_context *context = NULL;
    struct sdb_record* record, *update_record;

    res = sdb_init("testdb", &context);
    printf("sdb_init returned: %d\n", res);
    for(i = 0; i < sizeof(records)/sizeof(struct sdb_record); ++i) 
        res = sdb_insert(context, &records[i]);
    printf("sdb_insert returned: %d\n", res);

    /* update test */
    sdb_create_record("Billy", "Casarin", 1, &update_record);
    res = sdb_update(context, 1, update_record);
    sdb_destroy_record(update_record);

    sdb_create_record("B", "C", 1, &update_record);
    sdb_update(context, 1, update_record);
    sdb_destroy_record(update_record);

    /* this should fill the first orphaned record */
    sdb_create_record("Fill", "Orph1", 2, &update_record);
    sdb_insert(context, update_record);
    sdb_destroy_record(update_record);

    /* orphan tests */
    sdb_create_record("ORPHAN RAWR RAWR RAWR", "WHYYYYYY", 123, &update_record);
    sdb_update(context, 1, update_record);
    sdb_create_record("ORPHAN RAWR RAWR RAWRaa", "WHYYYYYY", 123, &update_record);
    sdb_update(context, 4, update_record);
    sdb_create_record("asdasdasdadsf","adsfadfasdfadsf", 1234556, &update_record);
    sdb_update(context, 5, update_record);

    printf("sdb_update returned: %d\n", res);

    for(i = 1; i <= 6; ++i)
    {
        res = sdb_find(context, i, &record, &offset);
        printf("sdb_find(%d) returned record: \n"
              "first: %s\nlast: %s\ndate: %d\n\n", i,
              record->first_name, record->last_name, record->date);
        sdb_destroy_record(record);
    }
    return 0;
}

