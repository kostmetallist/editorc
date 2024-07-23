#ifndef __ADV_BIDIR_LISTS_H__
#define __ADV_BIDIR_LISTS_H__


/*struct bidir_item_lst
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

} Bidir_lst;*/



int adjust_bckn(Bidir_lst *listname, struct bidir_item_lst *patient);

void adjust_lst(Bidir_lst *list, long *global_str);

void edit_sym_list(Bidir_lst *list, long str_pos, long sym_pos, char what, long *global_str);

void insrt_one(Bidir_lst *list, long after_which, char *what, long *global_str);

void insrt_lst(Bidir_lst *list1, Bidir_lst *list2, long after_which);

void delete_times_list(Bidir_lst *list, long from_where, long times);

void delete_br(Bidir_lst *list, long range_1, long range_2, long *gl_str);


#endif /* ADV_BIDIR_LISTS_H */
