#ifndef __BIDIR_LISTS_FILE_INPUT_H__
#define __BIDIR_LISTS_FILE_INPUT_H__

struct bidir_item_lst
{
        char *body;
        struct bidir_item_lst *prev;
        struct bidir_item_lst *next;
	long leng;
};


typedef struct
{
        struct bidir_item_lst *head;
        struct bidir_item_lst *tail;

} Bidir_lst;


char *strread_file(FILE *fn, int *err_code, long *symb_number);

void bidir_lst_init(Bidir_lst *listname);

void bidir_lst_append(Bidir_lst *listname, char *data, long lenn);

void bidir_lst_swap(struct bidir_item_lst *el_1, struct bidir_item_lst *el_2);

void bidir_lst_erase(Bidir_lst *listname);

void bidir_lst_del_one(Bidir_lst *listname, long elem_num);

void bidir_lst_readall(Bidir_lst *listname);

int string_sortout(struct bidir_item_lst *item1, struct bidir_item_lst *item2);

int read_to_list(FILE *filename, Bidir_lst *list, long *gl_str_num);

int write_from_list(FILE *filename, Bidir_lst *list);

void read_times_list(Bidir_lst *list, long from_where, long times, long tabw);

#endif /* BIDIR_LISTS_FILE_INPUT_H */
