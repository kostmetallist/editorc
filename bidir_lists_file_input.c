#include <stdio.h>
#include <stdlib.h>
#include "bidir_lists_file_input.h"

#define BUF_SIZE     64
#define CHAR_SIZE    sizeof(char)


char *strread_file(FILE *fn, int *err_code, long *symb_number)
{
        int ch;
        int buf_count = 0;
        unsigned int mult = 1;
        unsigned long offset = 0;
        char *storage_link = NULL;
        storage_link = (char *) malloc(BUF_SIZE * sizeof(char));

        if (storage_link == NULL)
        {
                printf("No available memory\n");
                *err_code = -1;
                return NULL;
        }

        storage_link[0] = '\0';

        while ((ch=fgetc(fn)) != '\n')
        {
                if (buf_count > (BUF_SIZE-1))
                {
                        buf_count = 0;
                        mult++;
                        storage_link = (char *) realloc(storage_link, BUF_SIZE * mult * sizeof(char));

                        if (storage_link == NULL)
                        {
                                printf("No available memory\n");
                                *err_code = -1;
                                return NULL;
                        }
                }

                if (ch == EOF)
                {
                        /*printf("EOF REACHED\n");*/
                        *err_code = -2;
                        break;
                }

                storage_link[offset] = ch;
                (*symb_number)++;
                offset++;
                buf_count++;
        }

        if (buf_count > (BUF_SIZE-1))
        {
                storage_link = (char *) realloc
                (storage_link, BUF_SIZE * mult * CHAR_SIZE + CHAR_SIZE);

                if (storage_link == NULL)
                {
                        printf("No available memory\n");
                        return NULL;
                }
        }

        *(storage_link + offset) = '\0';
        return storage_link;
}


void bidir_lst_init(Bidir_lst *listname)
{
	listname -> head = NULL;
	listname -> tail = NULL;
}


void bidir_lst_append(Bidir_lst *listname, char *data, long lenn)
{
	struct bidir_item_lst *new_item;
	new_item = (struct bidir_item_lst *) malloc(sizeof(struct bidir_item_lst));

	if (listname -> head == NULL && listname -> tail == NULL)
	{	
		new_item -> body = data;
		new_item -> prev = NULL;
		new_item -> next = NULL;
		new_item -> leng = lenn;
		listname -> head = new_item;
		listname -> tail = new_item;
		return;
	}

	new_item -> body = data;
	new_item -> prev = listname -> tail;
	new_item -> next = NULL;
	new_item -> leng = lenn;
	listname -> tail -> next = new_item;
	listname -> tail = new_item;
}


void bidir_lst_swap(struct bidir_item_lst *el_1, struct bidir_item_lst *el_2)
{
	char *tmp = el_2 -> body;
	long tmp_2 = el_2 -> leng;
	el_2 -> body = el_1 -> body;
	el_2 -> leng = el_1 -> leng;
	el_1 -> body = tmp;
	el_1 -> leng = tmp_2;
}


void bidir_lst_erase(Bidir_lst *listname)
{
	struct bidir_item_lst *lst_tmp = listname -> head;
	struct bidir_item_lst *delet = NULL;

	while (lst_tmp != NULL)
	{
		delet = lst_tmp;
		lst_tmp = lst_tmp -> next;
		free(delet -> body);
		free(delet);
	}

	listname -> head = NULL;
	listname -> tail = NULL;
}


void bidir_lst_del_one(Bidir_lst *listname, long elem_num)
{
	struct bidir_item_lst *victim = listname -> head;
	long curr_num = 1;

	while (curr_num != elem_num)
	{
		victim = victim -> next;
		curr_num++;
	}

	if (victim == listname -> head && victim == listname -> tail)
	{
		free(victim -> body);
		free(victim);
		listname -> head = NULL;
		listname -> tail = NULL;
		return;
	}

	if (victim == listname -> head)
	{
		victim -> next -> prev = NULL;
		listname -> head = victim -> next;		
		free(victim -> body);
		free(victim);
		return;
	}

	if (victim == listname -> tail)
	{
		victim -> prev -> next = NULL;
		listname -> tail = victim -> prev;	
		free(victim -> body);
		free(victim);
		return;
	}

	victim -> prev -> next = victim -> next;
	victim -> next -> prev = victim -> prev;
	free(victim -> body);
	free(victim);
}


void bidir_lst_readall(Bidir_lst *listname)
{
	struct bidir_item_lst *tmp = listname -> head;

	printf("LIST CONTAINING ::\n");

	while (tmp != NULL)     /* was: tmp != listname -> tail */
	{
		int offset = 0;
		unsigned char elem = tmp -> body[offset];
		printf("<");

		while (elem != '\0')	/* was: ...!= '\n' */
		{
			putchar((int) elem);
			offset++;
			elem = tmp -> body[offset];
		}

		putchar((int) '\0'); /* was: (int) '\n' */
		printf("> | %ld", tmp -> leng);
		printf("\n");
		tmp = tmp -> next;
	}
}


int string_sortout(struct bidir_item_lst *item1, struct bidir_item_lst *item2)
{
	char el_1, el_2;
	int offset = 0;
	el_1 = item1 -> body[offset];
	el_2 = item2 -> body[offset];

	while (1)
	{
		if ((el_1 == '\n') && (el_2 == '\n'))   /* equal */
		{
			return 0;
		}

		if ((el_1 == '\n') && (el_2 != '\n'))
		{
			return 0;
		}

		if ((el_1 != '\n') && (el_2 == '\n'))
		{
			return 1;
		}

		if (el_1 > el_2)
		{
			return 1;
		}

		if (el_1 < el_2)
		{
			return 0;
		}

		if (el_1 == el_2)
		{
			offset++;
			el_1 = item1 -> body[offset];
			el_2 = item2 -> body[offset];
			continue;
		}
	}
}


int read_to_list(FILE *filename, Bidir_lst *list, long *gl_str_num)
{
    char *file_string = NULL;

    while (1)
    {
        int err = 0;
        long symb_number = 0;

        file_string = strread_file(filename, &err, &symb_number);

        if (err == -1)
        {
            printf("Some problems with memory allocating.\n");
            return -1;
        }

        if (err == -2)
        {
            free(file_string);
            break;
        }

        bidir_lst_append(list, file_string, symb_number);
        (*gl_str_num)++;
    }

    return 0;
}


int write_from_list(FILE *filename, Bidir_lst *list)
{
    struct bidir_item_lst *current = list -> head;

    while (current != NULL)
    {
        int offset = 0;
        unsigned char elem = current -> body[offset];

        while (offset < (current -> leng))
        {
            if ((elem == '\r') && (current -> body[offset+1] == '\0'))
            {
                offset++;           /* This for [dos] files */
                elem = current -> body[offset];
                continue;
            }

            if (fputc((int) elem, filename) == EOF)
            {
                printf("Writing error.\n");
                return 1;
            }

            offset++;
            elem = current -> body[offset];
        }

        if (current != (list -> tail))
        {
            if (fputc((int) '\n', filename) == EOF)
            {
                printf("Writing error.\n");
                return 3;
            }
        }

        current = current -> next;
    }

    fputc((int) '\n', filename);
    return 0;
}


void read_times_list(Bidir_lst *list, long from_where, long times, long tabw)
{
    struct bidir_item_lst *tmp = list -> head;
    long curr_num = 1;

    while (curr_num != from_where)
    {
        tmp = tmp -> next;
        curr_num++;
    }

    curr_num = 0;

    while (curr_num < times)
    {
        int offset = 0;
        unsigned char elem = tmp -> body[offset];

        printf("\x1b[31;1m|\x1b[37m");

        while (elem != '\0')
        {
            if (elem == '\t')
            {
                long u = 0;

                while (u < tabw)
                {
                    putchar((int) ' ');
                    u++;
                }

                offset++;
                elem = tmp -> body[offset];
                continue;
            }

            putchar((int) elem);
            offset++;
            elem = tmp -> body[offset];
        }

        printf("\n");
        tmp = tmp -> next;
        curr_num++;
    }
}



