#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bidir_lists_file_input.h"
#include "adv_bidir_lists.h"

#define CHAR_SIZE    64


int adjust_bckn(Bidir_lst *listname, struct bidir_item_lst *patient)
{
        long rat = 0;
        struct bidir_item_lst *addit = NULL;
        char *before_n = NULL;
        char *after_n = NULL;

        while (rat < patient -> leng)
        {
                if (patient -> body[rat] == '\n')
                {
                        long mouse = 0;
                        long size_1, size_2;

                        addit = (struct bidir_item_lst *) malloc(sizeof(struct bidir_item_lst));
                        before_n = (char *) malloc(CHAR_SIZE * (rat + 1));

                        while (mouse < rat)
                        {
                                before_n[mouse] = patient -> body[mouse];
                                mouse++;
                        }

                        size_1 = mouse;
                        before_n[mouse] = '\0';
                        rat++;
                        mouse = 0;
                        size_2 = patient -> leng - rat;
                        after_n = (char *) malloc(CHAR_SIZE * (size_2 + 1));

                        while (rat <= patient -> leng)
                        {
                                after_n[mouse] = patient -> body[rat];
                                rat++;
                                mouse++;
                        }

                        free(patient -> body);
                        patient -> body = before_n;
                        patient -> leng = size_1;
                        addit -> body = after_n;
                        addit -> leng = size_2;

                        if (patient == listname -> tail)
                        {
                                listname -> tail = addit;
                                patient -> next = addit;
                                addit -> prev = patient;
                                addit -> next = NULL;
                                return 1;
                        }

                        patient -> next -> prev = addit;
                        addit -> next = patient -> next;
                        addit -> prev = patient;
                        patient -> next = addit;
                        return 1;
                }

                rat++;
        }

        return 0;       /* wasn't found any */
}



void adjust_lst(Bidir_lst *list, long *global_str)
{
        struct bidir_item_lst *tmp = list -> head;

        while (tmp != NULL)
        {
                (*global_str) += adjust_bckn(list, tmp);
                tmp = tmp -> next;
        }

        return;
}



void edit_sym_list(Bidir_lst *list, long str_pos, long sym_pos, char what, long *global_str)
{
        struct bidir_item_lst *tmp = list -> head;
        long curr_num = 1;

        while (curr_num != str_pos)
        {
                tmp = tmp -> next;
                curr_num++;
        }

        if (sym_pos > tmp -> leng)
        {
                printf("Incorrect symbol position - can't be greater than whole number of symbols in the string. Aborted.\n");
                return;
        }

        tmp -> body[sym_pos-1] = what;
        (*global_str) += adjust_bckn(list, tmp);
}



void insrt_one(Bidir_lst *list, long after_which, char *what, long *global_str)
{
        struct bidir_item_lst *tmp = list -> head;
        struct bidir_item_lst *new = NULL;
        long curr_num = 1;
        long what_leng = strlen(what);
        long indx = 0;
        char *new_body = NULL;

        new = (struct bidir_item_lst *) malloc(sizeof(struct bidir_item_lst));
        new_body = (char *) malloc(CHAR_SIZE * (what_leng + 1));
        (*global_str)++;

        while (indx < what_leng + 1)
        {
                new_body[indx] = what[indx];
                indx++;
        }

        new -> body = new_body;
        new -> leng = what_leng;

        if (after_which == 0)
        {
                if (list -> head == NULL && list -> tail == NULL)
                {
                        list -> head = new;
                        list -> tail = new;
                        new -> prev = NULL;
                        new -> next = NULL;
                        (*global_str) += adjust_bckn(list, new);
                        return;
                }

                new -> next = list -> head;
                new -> prev = NULL;
                list -> head -> prev = new;
                list -> head = new;
                (*global_str) += adjust_bckn(list, new);
                return;
        }

        while (curr_num != after_which)
        {
                tmp = tmp -> next;
                curr_num++;
        }

        if (tmp == list -> tail)
        {
                tmp -> next = new;
                new -> prev = tmp;
                new -> next = NULL;
                list -> tail = new;
                (*global_str) += adjust_bckn(list, new);
                return;
        }

        tmp -> next -> prev = new;
        new -> prev = tmp;
        new -> next = tmp -> next;
        tmp -> next = new;
        (*global_str) += adjust_bckn(list, new);
}



void insrt_lst(Bidir_lst *list1, Bidir_lst *list2, long after_which)
{
        struct bidir_item_lst *tmp = list1 -> head;
        long curr_num = 1;

        if (after_which == 0)
        {
                if (list1 -> head == NULL)
                {
                        list1 -> head = list2 -> head;
                        list1 -> tail = list2 -> tail;
                        return;
                }

                if (list2 -> head == NULL)
                        return;

                list2 -> tail -> next = list1 -> head;
                list1 -> head -> prev = list2 -> tail;
                list1 -> head = list2 -> head;
                return;
        }

        while (curr_num != after_which)
        {
                tmp = tmp -> next;
                curr_num++;
        }

        if (tmp == list1 -> tail)
        {
                tmp -> next = list2 -> head;
                list2 -> head -> prev = tmp;
                list1 -> tail = list2 -> tail;
                return;
        }

        tmp -> next -> prev = list2 -> tail;
        list2 -> head -> prev = tmp;
        list2 -> tail -> next = tmp -> next;
        tmp -> next = list2 -> head;
}


void delete_times_list(Bidir_lst *list, long from_where, long times)
{
        struct bidir_item_lst *tmp = list -> head;
        struct bidir_item_lst *auxiliary;
        long curr_num = 1;

        if (!times)
                return;

        while (curr_num != from_where)
        {
                tmp = tmp -> next;
                curr_num++;
        }

        curr_num = 0;

        while (curr_num < times)
        {
                if (tmp == list -> head && tmp == list -> tail)
                {
                        free(tmp -> body);
                        free(tmp);
                        list -> head = NULL;
                        list -> tail = NULL;
                        return;
                }

                if (tmp == list -> head)
                {
                        tmp -> next -> prev = NULL;
                        list -> head = tmp -> next;
                        auxiliary = tmp;
                        tmp = tmp -> next;
                        free(auxiliary -> body);
                        free(auxiliary);
                        curr_num++;
                        continue;
                }

                if (tmp == list -> tail)
                {
                        tmp -> prev -> next = NULL;
                        list -> tail = tmp -> prev;
                        free(tmp -> body);
                        free(tmp);
                        return;
                }

                tmp -> prev -> next = tmp -> next;
                tmp -> next -> prev = tmp -> prev;
                auxiliary = tmp;
                tmp = tmp -> next;
                free(auxiliary -> body);
                free(auxiliary);
                curr_num++;
        }
}


void delete_br(Bidir_lst *list, long range_1, long range_2, long *gl_str)
{
        struct bidir_item_lst *moddf;
        struct bidir_item_lst *tmp = list -> head;
        long curr_num = 1;
        long offset = 0;
        long str_1, fst_part_len, ccat_border;
        char *instead = NULL;
        int ext_br, int_br_l, int_br_r, deleted;
        char elem;

        ext_br = int_br_l = int_br_r = str_1 = fst_part_len = ccat_border = deleted = 0;

        while (curr_num != range_1)
        {
                tmp = tmp -> next;
                curr_num++;
        }

        elem = tmp -> body[0];

        while (curr_num <= range_2)
        {
                moddf = tmp;

                while (elem != '{' && elem != '}' && elem != '\0')
                {
                        offset++;
                        elem = tmp -> body[offset];
                }

                if (elem == '\0')       /* i.e. second condition of while ^ */
                {
                        tmp = tmp -> next;

                        if (tmp == NULL)
                                break;

                        curr_num++;
                        offset = 0;
                        elem = tmp -> body[offset];
                        continue;
                }

                if (elem == '{')
                {
                        if (!ext_br)            /* if it is first left brace */
                        {
                                long offset_2 = 0;
                                char elem_2 = tmp -> body[0];

                                ext_br = 1;
                                str_1 = curr_num;
                                fst_part_len = offset;
                                instead = (char *) malloc(CHAR_SIZE * (fst_part_len + 1));

                                while (elem_2 != '{')
                                {
                                        instead[offset_2] = elem_2;
                                        offset_2++;
                                        elem_2 = tmp -> body[offset_2];
                                }

                                ccat_border = offset_2;
                                /*printf("%s\n", instead);*/    /* Valgrind argues because don't have
                                                                 * \0 at the moment. But this
                                                                 * construction was for debug
                                                                 * so doesn't matter */
                                offset++;
                                elem = tmp -> body[offset];
                                continue;
                        }

                        else                    /* else count internal ones */
                        {
                                int_br_l++;
                                offset++;
                                elem = tmp -> body[offset];
                                continue;
                        }
                }

                if (elem == '}' && ext_br)              /* i.e. we found '}' */
                {
                        if (int_br_l == int_br_r)       /* internal braces balanced */
                        {
                                long offset_3 = offset + 1;
                                char elem_3 = tmp -> body[offset_3];

                                ext_br = 0;
                                int_br_l = int_br_r = 0;
                                instead = (char *) realloc(instead,
                                CHAR_SIZE * (fst_part_len + 1 + tmp -> leng - offset));

                                while (elem_3 != '\0' && elem_3 != '\r')
                                {
                                        instead[ccat_border] = elem_3;
                                        offset_3++;
                                        ccat_border++;
                                        elem_3 = tmp -> body[offset_3];
                                }

                                instead[ccat_border] = '\0';
                                free(moddf -> body);
                                moddf -> body = instead;
                                moddf -> leng = strlen(instead);
                                delete_times_list(list, (str_1 - deleted), (curr_num - str_1));
                                deleted += curr_num - str_1;
                                (*gl_str) -= curr_num - str_1;
                                ccat_border = 0;

                                offset = 0;
                                tmp = moddf;
                                elem = tmp -> body[offset];
                                continue;
                        }

                        else
                        {
                                int_br_r++;
                                offset++;
                                elem = tmp -> body[offset];
                                continue;
                        }
                }

                offset++;
                elem = tmp -> body[offset];
        }

        if (ext_br)             /* i.e. we didn't matched the external '}' at all */
        {
                tmp = list -> head;
                curr_num = 1;

                while (curr_num != str_1 - deleted)
                {
                        tmp = tmp -> next;
                        curr_num++;
                }

                instead[ccat_border] = '\0';
                free(tmp -> body);
                tmp -> body = instead;
                tmp -> leng = strlen(instead);

                if (tmp != list -> tail)
                {
                        delete_times_list(list, (str_1 - deleted + 1), (*gl_str) - str_1 + deleted);
                        (*gl_str) = str_1 - deleted;
                }

                return;
        }
}
