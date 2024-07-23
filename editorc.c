#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>   /* For stat() */
#include <sys/stat.h>   /*            */

#include <signal.h>
#include <termios.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bidir_lists_file_input.h"
#include "adv_bidir_lists.h"

#define BUF_SIZE        16
#define CHAR_SIZE       sizeof(char)
#define CTRL_SYM_NUM    7
#define CMD_NUMBER      18

#define ERR_INV_ARG_NUM printf("Invalid argument number.\n")
#define	DBG             printf("[x]")


typedef struct general_condition
{	
	char *name;
	int ass_stat;
	int svd_stat;
	int opn_stat;
	int wrp_stat;
	int ent_del;
	long tabwidth;
	long str_numb;

} general_condition;


typedef struct char_info
{	
	struct bidir_item_lst *str;
	long pos;

} char_info;


int cwait = 1;				/* For ^C signal*/


char *strread_stdin(FILE *fn, int *err_code, int *ch_numb)
{
	int cch;
	int buf_count = 0;
	unsigned int mult = 1;          /* Multiplier for adjusting memory */
	unsigned long offset = 0;
	char *storage_link = NULL;
	storage_link = (char *) malloc(BUF_SIZE * CHAR_SIZE);

	if (storage_link == NULL)               /* (Either NULL or 0?)  */
	{
		printf("No available memory\n");
		*err_code = -1;
		return NULL;
	}

	storage_link[0] = '\0';

	while ((cch=fgetc(fn)) != '\n')
	{
		if (buf_count > (BUF_SIZE-1))
		{
			buf_count = 0;
			mult++;
			storage_link = (char *) realloc(storage_link, BUF_SIZE * mult * CHAR_SIZE);

			if (storage_link == NULL)
			{
				printf("No available memory\n");
				*err_code = -1;
				return NULL;
			}
		}

		if (cch == EOF)
		{
			*err_code = -2;
			break;
		}

		storage_link[offset] = cch;
		(*ch_numb)++;
		offset++;
		buf_count++;
	}

	if (buf_count > (BUF_SIZE-1))
	{
		storage_link = (char *) realloc(storage_link,
		BUF_SIZE * mult * CHAR_SIZE + CHAR_SIZE);

		if (storage_link == NULL)
		{
			printf("No available memory\n");
			return NULL;
		}
	}

	*(storage_link + offset) = '\0';
	(*ch_numb)++;
	return storage_link;
}


char *str_spaceout(char *string_link, int *ch_numb, unsigned int *quota_mode, int *triple_qts,
			unsigned int *quota_precense)
{
	char ch;
	char *new_place;
	int old_offset = 0;
	int new_offset = 0;
	unsigned int space_mode = 0;
	
	new_place = (char *) malloc(CHAR_SIZE * (*ch_numb) + CHAR_SIZE);

	while ((ch = string_link[old_offset]) != '\0')
	{		
		if ((ch == ' ' || ch == '\t') && ((*quota_mode) == 0))
		{
			space_mode = 1;
			old_offset++;
			continue;
		}

		else
		{
			if (ch == '#')
			{	
				if (*quota_mode)
				{
					new_place[new_offset] = '#';
					new_offset++;
					old_offset++;
					continue;
				}

				else
				{
					if (old_offset == 0 || space_mode == 1)
					{
						free(string_link);
						new_place[new_offset] = '\0';
						return new_place;
					}
				}
			}

			if ((ch == '\"') && (!new_offset || new_place[new_offset - 1] != '\\'))
			{
				if (string_link[old_offset+1] != '\0' &&
				    string_link[old_offset+2] != '\0' &&
				    string_link[old_offset+1] == '\"' &&
				    string_link[old_offset+2] == '\"' &&
				    string_link[old_offset+3] != '\"')
				{
					(*triple_qts)++;

					if ((*triple_qts) & 1)
					{
						new_place[new_offset] = ' ';
						new_offset++;
					}

					(*quota_mode) ^= 1;
					new_place[new_offset] = '\"';
					new_place[new_offset+1] = '\"';
					new_place[new_offset+2] = '\"';
					space_mode = 0;
					new_offset += 3;
					old_offset += 3;
					continue;
				}

				(*quota_precense)++;
				
				if ((*quota_precense) & 1)
				{
					if (!space_mode && old_offset)
					{
						printf("Invalid symbol position before quotes.\n");
						free(string_link);
						free(new_place);
						return NULL;
					}

					new_place[new_offset] = ' ';
					new_offset++;
				}

				else
				{
					if (string_link[old_offset+1] != ' ' &&
					    string_link[old_offset+1] != '\t' &&
					    string_link[old_offset+1] != '\0')
					{
						printf("Invalid symbol position after quotes.\n");
						free(string_link);
						free(new_place);
						return NULL;
					}
				}

				(*quota_mode) ^= 1;
				new_place[new_offset] = '\"';
				space_mode = 0;
				new_offset++;
				old_offset++;
				continue;
			}

			if (space_mode)
			{
				space_mode = 0;
				new_place[new_offset] = ' ';
				new_offset++;
			}

			new_place[new_offset] = string_link[old_offset];
			new_offset++;
		}
			
		old_offset++;
	}

	free(string_link);

	if ((*quota_precense) & 1)
	{
		printf("Quotes dissonance. Input is aborted.\n");
		free(new_place);
		return NULL;
	}

	new_place[new_offset] = '\0';	/* was needed for excepting input "<command><spam>" */
	return new_place;
}


char *tab_mod(char *from, long sizel, long tabw)
{
	char *new;
	char elem;
	long offset1 = 0;
	long offset2 = 0;
	int tab_number = 0;

	new = (char *) malloc(CHAR_SIZE * (sizel + 1));

	while ((elem = from[offset1]) != '\0')
	{
		if (elem == '\t')
		{	
			long hh = 0;

			tab_number++;
			new = (char *) realloc(new, CHAR_SIZE * (sizel + 1) + (tab_number * tabw));

			while (hh < tabw)
			{
				new[offset2] = ' ';
				offset2++;
				hh++;
			}

			offset1++;
		}

		else
		{
			new[offset2] = from[offset1];
			offset1++;
			offset2++;
		}
	}

	new[offset2] = '\0';
	return new;
}


char *str_bck_mod(char *from)
{
	char set[CTRL_SYM_NUM] = {'a', 'b', 't', 'n', 'v', 'f', 'r'};
	long sym_num = 0;
	long new_off;
	char chh = from[sym_num];
	char *new = NULL;

	while (chh != '\0')
	{
		sym_num++;
		chh = from[sym_num];
	}

	sym_num++;
	new = (char *) malloc(CHAR_SIZE * sym_num);
	new_off = 0;
	sym_num = 0;
		
	while ((chh = from[sym_num]) != '\0')
	{
		if (chh == '\\')
		{
			char chh_nxt = from[sym_num + 1];
			int j = CTRL_SYM_NUM - 1;
			int cont_stat = 0;

			while (j >= 0)
			{
				if (chh_nxt == set[j])
				{
					new[new_off] = 7 + j;
					new_off++;
					sym_num += 2;
					cont_stat = 1;
				}

				j--;
			}

			if (cont_stat)
			{
				continue;
			}

			if  (chh_nxt == '\\' || chh_nxt == '\'' ||
			     chh_nxt == '\"' || chh_nxt == '\?')
			{
				new[new_off] = chh_nxt;
				new_off++;
				sym_num += 2;
				continue;
			}

			new[new_off] = '\\';
			new_off++;
			sym_num++;
			continue;
			
		}

		new[new_off] = chh;
		new_off++;
		sym_num++;
	}
	
	new[new_off] = '\0';
	return new;
}


void strbck_lst(Bidir_lst *list)
{
	struct bidir_item_lst *tmp = list -> head;
	char *modified;
	
	while (tmp != NULL)
	{
		modified = str_bck_mod(tmp -> body);
		free(tmp -> body);
		tmp -> body = modified;
		tmp -> leng = strlen(modified);
		tmp = tmp -> next;
	}

	return;
}


char *str_copy_until(char *from, char until_what, char until_what_2, int *ch_counter, int *q_neq)
{
	char ch;
	char prev;
	char until_what_1 = until_what;
	int buf_count = 0;
	int begin_flag;
	int an_fl;
	unsigned int mult = 1;
	unsigned long offset = 0;
	char *to = (char *) malloc(BUF_SIZE * CHAR_SIZE);

	if (to == NULL)
	{
		printf("No available memory\n");
		return NULL;
	}

	to[0] = '\0';
	ch = from[0];
	prev = '\0';
	begin_flag = 1;
	an_fl = 0;

	/*printf("/%s\\\n", from);*/
	if ((ch == '\"') && (from == strstr(from, "\"\"\"")) && (begin_flag))
	{	
		until_what_1 = '\0';
		ch = from[3];
		an_fl = 3;
		begin_flag = 0;
		*ch_counter = -2;
	}

	while (ch != until_what_1 && ch != until_what_2)
	{
		if ((ch == '\"') && (begin_flag))
		{
			until_what_1 = '\"';
			(*ch_counter) += 2;
			ch = from[1];
			an_fl = 1;
			begin_flag = 0;
			*q_neq = 1;
			continue;
		}

		if (buf_count > (BUF_SIZE-1))
		{
			buf_count = 0;
			mult++;
			to = (char *) realloc(to, BUF_SIZE * mult * CHAR_SIZE);

			if (to == NULL)
			{
				printf("No available memory\n");
				return NULL;
			}
		}

		to[offset] = ch;
		(*ch_counter)++;
		offset++;
		buf_count++;
		prev = ch;
		ch = from[offset + an_fl];

		if (ch == '\"' && prev == '\\')
		{
			to[offset] = ch;
			(*ch_counter)++;
			offset++;
			buf_count++;
			prev = '\"';
			ch = from[offset + an_fl];
			begin_flag = 0;
			continue;
		}

		begin_flag = 0;
	}

	until_what_1 = until_what;

	if (buf_count > (BUF_SIZE-1))
	{
		to = (char *) realloc(to, BUF_SIZE * mult * CHAR_SIZE + CHAR_SIZE);

		if (to == NULL)
		{
			printf("No available memory\n");
			return NULL;
		}
	}
		
	to[offset] = '\0';	

	if (from[offset + an_fl + *q_neq] == '\0')
	{
		*ch_counter = -2;
	}

	(*ch_counter)++;
	return to;
}


void erase_cage(char *arg_cage[5], int arg_type[5])
{
	int i = 4;

	while (i > -1)
	{
		if (arg_cage[i] != NULL)
		{
			free(arg_cage[i]);
			arg_cage[i] = NULL;
		}

		arg_type[i] = 0;
		i--;
	}
}


int split_ord(char *begin, char *arg_cage[5], int arg_type[5])
{
	int arg_num = 0;
	int shift = 0;
	int q_flag;

	while (1)
	{
		q_flag = 0;

		if (begin[0] == '\0')
		{
			break;
		}

		arg_cage[arg_num] = str_copy_until(begin+shift+1, ' ', '\0', &shift, &q_flag);
		arg_type[arg_num] = q_flag;
		arg_num++;		/* Maybe I have to do ((arguments <= 5) check) */

		if (shift == -1)
		{
			break;
		}
	}

	return arg_num;
}


int alert_open_file(FILE *filename)
{
	if (filename == NULL)
	{
		printf("There are some problems while opening a file.\n");
		return 1;
	}

	return 0;
}


int alert_close_file(int fclose_fun)
{
	if (fclose_fun == EOF)
	{
		printf("There are some problems while closing a file.\n");
		return 1;
	}

	return 0;
}


int print_diap_no(Bidir_lst *list, long str_diap, long sym_diap, int w, int h, long tabw, long gl_str)
{
	/* if tabw == 0 => do not change \t to the spaces */

	struct bidir_item_lst *tmp = list -> head;
	long begin_str, begin_sym;
	long curr_num = 0;

	begin_str = str_diap * (h - 1);
	begin_sym = sym_diap * w;

	if (begin_str > gl_str)
	{
		return 1;	/* else we will print only whitespace */
	}

	while (curr_num != begin_str)
	{
		tmp = tmp -> next;
		curr_num++;
	}

	curr_num = 0;

	while (curr_num < (h - 1) && tmp != NULL)
	{
		int offset = begin_sym;
		char *tabbed;
		unsigned char elem;

		if (!tabw)
			tabbed = tmp -> body;

		else
			tabbed = tab_mod(tmp->body, tmp->leng, tabw);

		if (begin_sym > strlen(tabbed))
		{
			printf("\n");

			if (tabw)
				free(tabbed);

			tmp = tmp -> next;
			curr_num++;
			continue;
		}

		elem = tabbed[offset];

		while (elem != '\0' && offset < (begin_sym + w))
		{
			putchar((int) elem);
			offset++;
			elem = tabbed[offset];
		}
	
		printf("\n");

		if (tabw)
			free(tabbed);

		tmp = tmp -> next;
		curr_num++;
	}
	
	return 0;
}


void print_wrap(char_info *info, int w, int h, long tabw)
{
	/* if tabw == 0 => do not change \t to the spaces */

	long printed_str = 0;
	long offset = info -> pos;
	struct bidir_item_lst *item = info -> str;
	int printed_sym;
	int is_new_line = 1;
	char *tabbed = NULL;

	while (printed_str < (h - 1) && item != NULL)
	{	
		unsigned char elem;

		printed_sym = 0;

		if (!offset)
		{
			printf("\x1b[31;1m|\x1b[37m");
			printed_sym++;
		}

		if (is_new_line)
		{
			if (!tabw)
				tabbed = item -> body;

			else
				tabbed = tab_mod(&(item->body[offset]), (item->leng - offset), tabw);

			is_new_line = 0;
			offset = 0;
		}

		elem = tabbed[offset];

		while (elem != '\0' && printed_sym < w)
		{
			putchar((int) elem);
			printed_sym++;
			offset++;
			elem = tabbed[offset];
		}

		if (printed_sym == w)	/* i.e. second condition from while ^ */
		{
			printf("\n");
			printed_str++;
			continue;
		}

		printf("\n");

		if (tabw)
		{
			free(tabbed);
			tabbed = NULL;
		}

		item = item -> next;
		printed_str++;
		offset = 0;
		is_new_line = 1;
	}

	if (tabbed != NULL)
		free(tabbed);
	
	info -> str = item;
	info -> pos = offset;
}


char *str_concat(const int mode, char *the_1, char *the_2)
{
	/* mode == 0 => concatenate second string to the beginning of the first;
	 * mode == 1 => ---||--- to the end
	 */

	long new_len;
	long offset;
	char *new_str;
	char elem;
 
	new_len = strlen(the_1) + strlen(the_2);
	new_str = (char *) malloc(CHAR_SIZE * (new_len + 1));

	if (!mode)
	{
		long offset_2 = 0;

		offset = 0;
		elem = the_2[offset];

		while (elem != '\0')
		{
			new_str[offset_2] = elem;
			offset++;
			offset_2++;
			elem = the_2[offset];
		}

		offset = 0;
		elem = the_1[offset];

		while (elem != '\0')
		{
			new_str[offset_2] = elem;
			offset++;
			offset_2++;
			elem = the_1[offset];
		}

		new_str[offset_2] = '\0';	
		return new_str;
	}

	if (mode == 1)
	{
		long offset_2 = 0;

		offset = 0;
		elem = the_1[offset];

		while (elem != '\0')
		{
			new_str[offset] = elem;
			offset++;
			elem = the_1[offset];
		}

		offset_2 = offset;
		offset = 0;
		elem = the_2[offset];

		while (elem != '\0')
		{
			new_str[offset_2] = elem;
			offset++;
			offset_2++;
			elem = the_2[offset];
		}

		new_str[offset_2] = '\0';
		return new_str;
	}

	else
	{
		printf("Incorrect argument 1. Look for documentation.\n");
		free(new_str);
		return NULL;
	}
}


char *str_replace(char *general, char *outcome, char *income, long len_1, long len_2, long *point)
{	
	/* if either len_1 or len_2 is equal to 0, we'll calculate 
	 * lengths right there
	 */

	long difference, start_len, internal_count, out_len, in_len;
	char *dest;
	char *new_memory;
	int need_mem;

	start_len = strlen(general);

	if (len_1 || len_2)
	{
		out_len = len_1;
		in_len = len_2;
	}

	else
	{
		out_len = strlen(outcome);
		in_len = strlen(income);
	}

	need_mem = (out_len < in_len) ? 1 : 0;
	difference = need_mem ? (in_len - out_len) : (out_len - in_len);
	internal_count = 0;

	if ((dest = strstr(&general[*point], outcome)) != NULL)
	{
		long offset;
		long k, l;
		char elem;	

		k = 0;
		elem = dest[0];

		while (elem != '\0')
		{
			k++;
			elem = dest[k];
		}

		new_memory = (char *) malloc(CHAR_SIZE * (start_len + 1 + difference));	
		offset = start_len - k;
		k = 0;
		
		while (internal_count < offset)
		{
			new_memory[internal_count] = general[k];
			internal_count++;
			k++;
		}

		l = 0;

		while ((elem = income[l]) != '\0')
		{
			new_memory[internal_count] = elem;
			l++;
			internal_count++;
		}

		offset += out_len;
		*point = internal_count;

		while ((elem = general[offset]) != '\0')
		{
			new_memory[internal_count] = elem;
			offset++;
			internal_count++;
		}

		new_memory[internal_count] = '\0';
		return new_memory;
	}

	return NULL;
}


void inter_handler(int sig_num)
{
	printf("\nKeyboard interrupt.\n");
	cwait = 0;
}




/* ___________________________________________________________________________________________ */

int main(int argc, char **argv)
{
	struct winsize sz;
	int width, height, is_terminal_out, cmd_cursor, width_iter;
	char *arg_cage[5] = {NULL, NULL, NULL, NULL, NULL};
	int arg_type[5] = {0, 0, 0, 0, 0};
	char *command_database[CMD_NUMBER] = {"set tabwidth", "print pages", "print range",
					     "set wrap", "insert after", "edit string",
					     "insert symbol", "replace substring", "delete range",
					     "delete braces", "exit", "read", "open", "write",
					     "set name", "help", "directory", "status"};
	char control_set[CTRL_SYM_NUM] = {'a', 'b', 't', 'n', 'v', 'f', 'r'};
	char *cmd_place = NULL;
	general_condition g_cond;
	Bidir_lst global_text;
	Bidir_lst help_box;
	Bidir_lst *entered_copy;
	FILE *filename_1 = NULL;

	g_cond.wrp_stat = 1;
	g_cond.tabwidth = 8;
	g_cond.ent_del = 1;
	bidir_lst_init(&global_text);
	bidir_lst_init(&help_box);
	signal(SIGINT, inter_handler);

	is_terminal_out = isatty(1) ? 1 : 0;
	ioctl(0, TIOCGWINSZ, &sz);
	width = sz.ws_col;
	height = sz.ws_row;
	width_iter = 0;

	while (width_iter < width)
	{
		printf("\x1b[31;1m#\x1b[37m");
		width_iter++;
	}

	printf("\n");
	
	switch (argc)
	{
		int pid_0 = -1;
		int touch_status;
		struct stat statb;

		case 1:
			g_cond.name = NULL;
			g_cond.ass_stat = 0;
			g_cond.svd_stat = 1;
			g_cond.opn_stat = 0;
			g_cond.str_numb = 0;
			break;

		default:
			g_cond.name = argv[1];
			g_cond.ass_stat = 1;
			g_cond.svd_stat = 1;
			g_cond.str_numb = 0;

			if (stat(argv[1], &statb) == -1)
			{
				printf("Not found such file. Created new one.\n");
				g_cond.opn_stat = 0;
				pid_0 = fork();

				if (pid_0 == -1)
				{
					perror("Unsuccessful creating. Sorry 'bout that.\n");
					break;
				}

				if (!pid_0)
				{
					execlp("touch", "touch", argv[1], NULL);
					perror("Started but failed creation. Sorry.\n");
					break;
				}

				wait(&touch_status);
				break;
			}

			else
			{
				printf("Found. Reading...\n");
				filename_1 = fopen(argv[1], "r");

				if (alert_open_file(filename_1))
					return 1;

				g_cond.opn_stat = 1;
				read_to_list(filename_1, &global_text, &g_cond.str_numb);

				if (alert_close_file(fclose(filename_1)))
					return 2;

				g_cond.opn_stat = 0;
				break;
			}
	}

	while (cwait)		/* External while */
	{
		int begin_triple_qts = 0;
		int triple_qts = 0;
		int further = 0;
		int sec_str_input = 0;
		int is_first = 1;
		int entered_len = 0;
		unsigned int quota_precense;
		unsigned int quota_mode = 0;
		Bidir_lst entered_str;

		bidir_lst_init(&entered_str);
		printf("\n\x1b[31;1mEditorC: \x1b[37m");

		while (1)
		{
			int err_code = 0;
			int ch_numb = 0;
			char *curr_string;

			quota_precense = 0;	
			curr_string = str_spaceout
			 (strread_stdin(stdin, &err_code, &ch_numb),
			  &ch_numb, &quota_mode, &triple_qts, &quota_precense);

			if (err_code == -2 && cwait)		/* -_- is it right solution ? */
			{
				printf("^D\n");

				if (g_cond.str_numb)
					bidir_lst_erase(&global_text);

				if (g_cond.name != NULL &&
					g_cond.name != argv[1])
					free(g_cond.name);

				free(curr_string);
				erase_cage(arg_cage, arg_type);
				return 0;
			}

			if (curr_string == NULL)
			{
				goto ForceContinue;
			}

			entered_len++;
			ch_numb--;

			if (is_first && (triple_qts == 1))
				begin_triple_qts = 1;

			if (strstr(curr_string, "insert after") && !quota_precense && is_first)
			{
				/*printf("[Sec_str invoked]\n");*/
				sec_str_input = 1;
				bidir_lst_append(&entered_str, curr_string, ch_numb);
				continue;
			}

			is_first = 0;

			if (further && quota_precense)	/* if quotas not in the first string */
			{
				printf("Quotes notation error. Type '\\\"' for inserting into another quotes. Aborted.\n");
				goto ForceContinue;
			}

			bidir_lst_append(&entered_str, curr_string, ch_numb);
			further = (triple_qts & 1) ? 1 : 0;		

			if (further == 0)
			{
				break;
			}
		}

		/*bidir_lst_readall(&entered_str);*/
		entered_copy = &entered_str;

	cmd_cursor = 0;
	
	while (cmd_cursor < CMD_NUMBER)
	{
		cmd_place = strstr(entered_str.head->body, command_database[cmd_cursor]);

		if ((cmd_place != NULL) && ((cmd_place == entered_str.head->body) ||
		((cmd_place == entered_str.head->body+1) && (entered_str.head->body[0] == ' '))))
		{	
			if (cmd_place[strlen(command_database[cmd_cursor])] == ' ' ||
			cmd_place[strlen(command_database[cmd_cursor])] == '\0')
			{
				break;
			}

			else cmd_cursor = CMD_NUMBER - 1;
		}

		cmd_cursor++;
	}

	if (cmd_cursor == CMD_NUMBER)
	{
		if (cwait)
			printf("Incorrect command. Type \"help\" for advice.\n");

		bidir_lst_erase(&entered_str);
		continue;
	}
	
	else
	{
		/*printf("Recognized! It is \"%s\".\n", command_database[cmd_cursor]);*/

		switch (cmd_cursor)
		{
			char *aux_str_1;
			int ls_status;
			int aux_int = -1;
			int aux_int_arg = 0;
			/*int KK = 0;*/

			case 0:		/* set tabwidth */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					long strtoul_res;
					char *endptr;

					case 1:
						strtoul_res = strtoul(arg_cage[0], &endptr, 10);

						if (endptr[0] != '\0' || arg_type[0])
						{
							printf("Argument must be a digital value and without quotes. Aborted.\n");
							break;
						}

						printf("It is <%ld>\n", strtoul_res);

						if (strtoul_res <= 0)
						{
							printf("Invalid argument. Must be greater than zero. Aborted.\n");
							break;
						}

						g_cond.tabwidth = strtoul_res;
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}
		
				break;

			case 1:		/* print pages */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);	

				switch (aux_int_arg)
				{

				case 0:

				if (g_cond.wrp_stat)
				{
					struct termios old_attr, new_attr;
					char_info particle;
					int sym_c;

					if (!isatty(0))
					{
						printf("Input is not from terminal.");
						return 4;
					}

					particle.str = global_text.head;
					particle.pos = 0;

					if (!is_terminal_out)
					{
						while (particle.str != NULL)
						{
							print_wrap(&particle, width, height,
							0);
						}
					}

					else
					{

					tcgetattr(0, &old_attr);
					memcpy(&new_attr, &old_attr, sizeof(struct termios));
					new_attr.c_lflag &= ~ECHO;
					new_attr.c_lflag &= ~ICANON;
					new_attr.c_cc[VMIN] = 1;
					tcsetattr(0, TCSANOW, &new_attr);
					print_wrap(&particle, width, height,
					g_cond.tabwidth);
					printf("\x1b[31;1m[Space Q]\x1b[37m");
					sym_c = '\0';	

					while ((sym_c = getchar()) != 'q')
					{
						int pid_s = -1;
						int clr_st;	
						
						if (sym_c == ' ')
						{
							pid_s = fork();

	       		                        	if (pid_s == -1)
       		                        		{
       	        	                        		perror("Unsuccessful clear.\n");
               	        	                		break;
                       	        			}

                               				if (!pid_s)
	                               			{
        	                               			execlp("clear", "clear", NULL);
                	                       			perror("Entered but failed clear.\n");
                        	               			break;
                               				}

							wait(&clr_st);
						
							if (particle.str != NULL)
							{
								print_wrap(&particle, width, height,
								g_cond.tabwidth);
								printf("\x1b[31;1m[Space Q]\x1b[37m");
								continue;
							}

							else
								break;
						}
					}

					tcsetattr(0, TCSANOW, &old_attr);

					}
				}

				else	/* wrap no */
				{
					struct termios old_attr, new_attr;
					int sym_c;
					long str_diap;
					long sym_diap;

					if (!isatty(0))
					{
						printf("Input is not from terminal.");
						return 4;
					}

					str_diap = sym_diap = 0;

					if (!is_terminal_out)
					{	
						while (str_diap != (g_cond.str_numb / height))
						{
							print_diap_no(&global_text, str_diap,
							sym_diap, width, height, 0,
							g_cond.str_numb);
							str_diap++;
						}
					}

					else
					{

					tcgetattr(0, &old_attr);
					memcpy(&new_attr, &old_attr, sizeof(struct termios));
					new_attr.c_lflag &= ~ECHO;
					new_attr.c_lflag &= ~ICANON;
					new_attr.c_cc[VMIN] = 1;
					tcsetattr(0, TCSANOW, &new_attr);
					sym_c = '\0';

					while (sym_c != 'q')
					{
						int pid_c = -1;
						int clear_status;

						pid_c = fork();

						if (pid_c == -1)
						{
							perror("Unsuccessful clear.\n");
							break;
						}

						if (!pid_c)
						{
							execlp("clear", "clear", NULL);
							perror("Entered but failed clear.\n");
							break;
						}

						wait(&clear_status);

						if (sym_c == -1)	/* catching ^C */
							break;

						if (sym_c == ',')
						{
							if (sym_diap != 0)
								sym_diap--;
						}

						if (sym_c == '.')
							sym_diap++;

						if (sym_c == ' ')
						{
							if (str_diap == (g_cond.str_numb / height))
								break;

							str_diap++;
						}
						
						if (sym_c == 'b')
						{
							if (str_diap != 0)
								str_diap--;
						}

						print_diap_no(&global_text, str_diap, sym_diap, width,
						height, g_cond.tabwidth, g_cond.str_numb);
						printf("\x1b[31;1m[Space B < > Q]\x1b[37m");

						if (sym_c == '\004')
							break;

						sym_c = getchar();
					}

					tcsetattr(0, TCSANOW, &old_attr);
					
					} /* else */

					break;

				default:
						ERR_INV_ARG_NUM;
						break;
				}
				}

				break;

			case 2:		/* print range */
				
				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);	

				switch (aux_int_arg)
				{
					long fr_place;
					long to_place;
					long how_many;
					char *endptr;

					case 2:
						fr_place = strtoul(arg_cage[0], &endptr, 10);

						if (endptr[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}
	
						to_place = strtoul(arg_cage[1], &endptr, 10);

						if (endptr[0] != '\0' || arg_type[1])
						{
							printf("Argument 2 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (fr_place < 1 || to_place < 1)
						{
							printf("Arguments can't be less than 1. Aborted.\n");
							break;
						}

						if (fr_place > to_place)
						{
							printf("Incorrect argument order. Second value must be greater or equal to the first. Aborted.\n");
							break;
						}

						if (to_place > g_cond.str_numb)
						{
							printf("Invalid second value - bigger than general string number. Aborted.\n");
							break;
						}

						how_many = to_place - fr_place + 1;
						read_times_list(&global_text, fr_place, how_many,
						g_cond.tabwidth);
						break;

					case 1:
						fr_place = strtoul(arg_cage[0], &endptr, 10);

						if (endptr[0] != '\0' || arg_type[0])
						{
							printf("Argument must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (fr_place < 1)
						{
							printf("Argument can't be less than 1. Aborted.\n");
							break;
						}

						if (fr_place > g_cond.str_numb)
						{
							printf("Invalid value - bigger than general string number. Aborted.\n");
							break;
						}
						
						how_many = g_cond.str_numb - fr_place + 1;
						read_times_list(&global_text, fr_place, how_many,
						g_cond.tabwidth);
						break;

					case 0:
						read_times_list(&global_text, 1, g_cond.str_numb,
						g_cond.tabwidth);
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 3:		/* set wrap */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);	

				switch (aux_int_arg)
				{
					case 1:
						if (!strcmp(arg_cage[0], "yes") &&
						!arg_type[0])
						{
							g_cond.wrp_stat = 1;
							break;
						}

						if (!strcmp(arg_cage[0], "no") &&
						!arg_type[0])
						{
							g_cond.wrp_stat = 0;
							break;
						}

						printf("Invalid argument.\n");
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 4:		/* insert after */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);
				
				if (sec_str_input == 0)
				{

				switch(aux_int_arg)
				{
					long after_place;
					char *endptr_2;
					char *intermediate = NULL;

					case 2:
						after_place = strtoul(arg_cage[0], &endptr_2, 10);

						if (endptr_2[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (after_place < 0)
						{
							printf("Argument 1 can't be less than 0. Aborted.\n");
							break;
						}

						if (after_place > g_cond.str_numb)
						{
							printf("Invalid value - bigger than general string number. Aborted.\n");	
							break;
						}

						if (arg_type[1])
						{	
							intermediate = str_bck_mod(arg_cage[1]);
							insrt_one(&global_text, after_place,
							intermediate, &g_cond.str_numb);
							adjust_lst(&global_text, &g_cond.str_numb);
							free(intermediate);
							g_cond.svd_stat = 0;
							break;
						}

						else
						{
							printf("Argument 2 must be a string in quotes. Aborted.\n");
							break;
						}

					case 1:
						if (arg_type[0])
						{
							intermediate = str_bck_mod(arg_cage[0]);
							after_place = g_cond.str_numb;
							insrt_one(&global_text, after_place,
							intermediate, &g_cond.str_numb);
							adjust_lst(&global_text, &g_cond.str_numb);
							free(intermediate);
							g_cond.svd_stat = 0;
							break;
						}

						else
						{
							printf("Argument 2 must be a string in quotes. Aborted.\n");
							break;
						}

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

				} /* endif sec_str_input == 0 */


				else /* multiinput mode */
				{
				
				switch (aux_int_arg)
				{
					long after_place2;
					long part_offset;
					char *endptr_3;
					char *fst_part = NULL;
					char *sec_part = NULL;
					char gear;
					int fst_count;

					case 2:	/* insert after <value0> """<value1> */

						after_place2 = strtoul(arg_cage[0], &endptr_3, 10);

						if (endptr_3[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (after_place2 < 0)
						{
							printf("Argument 1 can't be less than 0. Aborted.\n");
							break;
						}

						if (after_place2 > g_cond.str_numb)
						{
							printf("Invalid value - bigger than general string number. Aborted.\n");
							break;
						}

						part_offset = 0;
						fst_part = (char *) malloc
						(CHAR_SIZE * strlen(arg_cage[1]) + 1);

						while ((gear = arg_cage[1][part_offset]) != '\0')
						{
							fst_part[part_offset] = gear;
							part_offset++;
						}

						fst_part[part_offset] = '\0';
						free(entered_str.head -> body);
						entered_str.head -> body = fst_part;
						entered_str.head -> leng = strlen(arg_cage[1]);

						part_offset = 0;
						sec_part = (char *) malloc
						(CHAR_SIZE * (entered_str.tail -> leng) + 1);

						while ((gear = entered_str.tail -> 
						body[part_offset]) != '\0')
						{
							if (gear == '\"' &&
							entered_str.tail -> 
							body[part_offset + 1] == '\"' &&
							entered_str.tail ->
							body[part_offset + 2] == '\"' &&
							entered_str.tail ->
							body[part_offset + 3] == '\0')
							{
								break;
							}

							sec_part[part_offset] = gear;
							part_offset++;
						}

						sec_part[part_offset] = '\0';
						free(entered_str.tail -> body);
						entered_str.tail -> body = sec_part;
						entered_str.tail -> leng = part_offset;
						strbck_lst(&entered_str);
						adjust_lst(&entered_str, &g_cond.str_numb);	
						insrt_lst(&global_text, &entered_str, after_place2);
						g_cond.str_numb += entered_len;
						g_cond.ent_del = 0;
						g_cond.svd_stat = 0;
						break;

					case 1:	/* insert after """<value0> */

						if (begin_triple_qts)
						{

						after_place2 = g_cond.str_numb;
						part_offset = 0;
						fst_part = (char *) malloc
						(CHAR_SIZE * strlen(arg_cage[0]) + 1);

						while ((gear = arg_cage[0][part_offset]) != '\0')
						{
							fst_part[part_offset] = gear;
							part_offset++;
						}

						fst_part[part_offset] = '\0';
						free(entered_str.head -> body);
						entered_str.head -> body = fst_part;
						entered_str.head -> leng = strlen(arg_cage[0]);

						part_offset = 0;
						sec_part = (char *) malloc
						(CHAR_SIZE * (entered_str.tail -> leng) + 1);

						while ((gear = entered_str.tail -> 
						body[part_offset]) != '\0')
						{
							if (gear == '\"' &&
							entered_str.tail -> 
							body[part_offset + 1] == '\"' &&
							entered_str.tail ->
							body[part_offset + 2] == '\"' &&
							entered_str.tail ->
							body[part_offset + 3] == '\0')
							{
								break;
							}

							sec_part[part_offset] = gear;
							part_offset++;
						}

						sec_part[part_offset] = '\0';
						free(entered_str.tail -> body);
						entered_str.tail -> body = sec_part;
						entered_str.tail -> leng = part_offset;
						strbck_lst(&entered_str);
						adjust_lst(&entered_str, &g_cond.str_numb);	
						insrt_lst(&global_text, &entered_str, after_place2);
						g_cond.str_numb += entered_len;
						g_cond.ent_del = 0;
						g_cond.svd_stat = 0;
						break;

						}

						else
						{

						/* insert after <value 0> */	
						
						after_place2 = strtoul(arg_cage[0], &endptr_3, 10);

						if (endptr_3[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (after_place2 < 0)
						{
							printf("Argument 1 can't be less than 0. Aborted.\n");
							break;
						}

						if (after_place2 > g_cond.str_numb)
						{
							printf("Invalid value - bigger than general string number. Aborted.\n");
							break;
						}

						entered_str.head = entered_str.head -> next;
						free(entered_str.head -> prev -> body);
						free(entered_str.head -> prev);
						entered_str.head -> prev = NULL;	
						entered_len--;

						if (entered_str.head -> body[0] != ' '  ||
						    entered_str.head -> body[1] != '\"' ||
						    entered_str.head -> body[2] != '\"' ||
						    entered_str.head -> body[3] != '\"')
						{
							printf("You should put text into triple quotes. Aborted.\n");
							break;
						}

						part_offset = 4;
						fst_part = (char *) malloc
						(CHAR_SIZE * (entered_str.head -> leng));
						fst_count = 0;

						while ((gear = entered_str.head -> body[part_offset])
						!= '\0')
						{
							fst_part[part_offset - 4] = gear;
							part_offset++;
							fst_count++;
						}

						fst_part[part_offset - 4] = '\0';
						free(entered_str.head -> body);
						entered_str.head -> body = fst_part;
						entered_str.head -> leng = fst_count;
						part_offset = 0;
						sec_part = (char *) malloc
						(CHAR_SIZE * (entered_str.tail -> leng) + 1);

						while ((gear = entered_str.tail -> 
						body[part_offset]) != '\0')
						{
							if (gear == '\"' &&
							entered_str.tail -> 
							body[part_offset + 1] == '\"' &&
							entered_str.tail ->
							body[part_offset + 2] == '\"' &&
							entered_str.tail ->
							body[part_offset + 3] == '\0')
							{
								break;
							}

							sec_part[part_offset] = gear;
							part_offset++;
						}

						sec_part[part_offset] = '\0';
						free(entered_str.tail -> body);
						entered_str.tail -> body = sec_part;
						entered_str.tail -> leng = part_offset;
						strbck_lst(&entered_str);
						adjust_lst(&entered_str, &g_cond.str_numb);
						insrt_lst(&global_text, &entered_str, after_place2);
						g_cond.str_numb += entered_len;
						g_cond.ent_del = 0;
						g_cond.svd_stat = 0;
						break;

						}

					case 0:
						after_place2 = g_cond.str_numb;
						entered_str.head = entered_str.head -> next;
						free(entered_str.head -> prev -> body);
						free(entered_str.head -> prev);
						entered_str.head -> prev = NULL;	
						entered_len--;

						if (entered_str.head -> body[0] != ' '  ||
						    entered_str.head -> body[1] != '\"' ||
						    entered_str.head -> body[2] != '\"' ||
						    entered_str.head -> body[3] != '\"')
						{
							printf("You should put text into triple quotes. Aborted.\n");
							break;
						}

						part_offset = 4;
						fst_part = (char *) malloc
						(CHAR_SIZE * (entered_str.head -> leng));
						fst_count = 0;

						while ((gear = entered_str.head -> body[part_offset])
						!= '\0')
						{
							fst_part[part_offset - 4] = gear;
							part_offset++;
							fst_count++;
						}

						fst_part[part_offset - 4] = '\0';
						free(entered_str.head -> body);
						entered_str.head -> body = fst_part;
						entered_str.head -> leng = fst_count;

						part_offset = 0;
						sec_part = (char *) malloc
						(CHAR_SIZE * (entered_str.tail -> leng) + 1);

						while ((gear = entered_str.tail -> 
						body[part_offset]) != '\0')
						{
							if (gear == '\"' &&
							entered_str.tail -> 
							body[part_offset + 1] == '\"' &&
							entered_str.tail ->
							body[part_offset + 2] == '\"' &&
							entered_str.tail ->
							body[part_offset + 3] == '\0')
							{
								break;
							}

							sec_part[part_offset] = gear;
							part_offset++;
						}

						sec_part[part_offset] = '\0';
						free(entered_str.tail -> body);
						entered_str.tail -> body = sec_part;
						entered_str.tail -> leng = part_offset;
						strbck_lst(&entered_str);
						adjust_lst(&entered_str, &g_cond.str_numb);	
						insrt_lst(&global_text, &entered_str, after_place2);
						g_cond.str_numb += entered_len;
						g_cond.ent_del = 0;
						g_cond.svd_stat = 0;
						break;	

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

				} /* end multiinput mode */


			case 5:		/* edit string */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					long edit_str;
					long edit_elem;
					char *endptr0;

					case 3:
						edit_str = strtoul(arg_cage[0], &endptr0, 10);

						if (endptr0[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}
	
						edit_elem = strtoul(arg_cage[1], &endptr0, 10);

						if (endptr0[0] != '\0' || arg_type[1])
						{
							printf("Argument 2 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (edit_str < 1 || edit_elem < 1)
						{
							printf("Arguments can't be less than 1. Aborted.\n");
							break;
						}

						if (edit_str > g_cond.str_numb)
						{
							printf("Invalid first value - bigger than general string number. Aborted.\n");
							break;
						}

						if (strlen(arg_cage[2]) > 2)
						{
							printf("Invalid symbol notation - given more than 2 bytes. Aborted.\n");
							break;
						}

						if (strlen(arg_cage[2]) == 1)
						{	
							edit_sym_list(&global_text, edit_str,
							edit_elem, arg_cage[2][0], &g_cond.str_numb);
							g_cond.svd_stat = 0;
							break;
						}

						else
						{
							if (arg_cage[2][0] != '\\')
							{
								printf("Invalid special symbol notation. Aborted.\n");
								break;
							}

							else
							{
								int k = CTRL_SYM_NUM - 1;
								int edit_further = 0;
								char what_edit;
								char change_sym = arg_cage[2][1];
								
								while (k >= 0)
								{
									if (change_sym == 
									control_set[k])
									{
										what_edit = 7 + k;
										edit_further = 1;
									}

									k--;
								}

								if (edit_further)
								{
									edit_sym_list(&global_text,
									edit_str, edit_elem,
									what_edit, &g_cond.str_numb);
									g_cond.svd_stat = 0;
									break;
								}

								if (change_sym == '\\' ||
								    change_sym == '\'' ||
								    change_sym == '\"' ||
								    change_sym == '\?')
								{
									what_edit = change_sym;
									edit_sym_list(&global_text,
									edit_str, edit_elem,
									what_edit, &g_cond.str_numb);
									g_cond.svd_stat = 0;
									break;
								}
							}
						}

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 6:		/* insert symbol */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					long ins_str;
					long ins_elem;
					char *endptr01;
					char *changed = NULL;
					struct bidir_item_lst *objct;
					long objct_num;

					case 3:
						ins_str = strtoul(arg_cage[0], &endptr01, 10);

						if (endptr01[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}
		
						ins_elem = strtoul(arg_cage[1], &endptr01, 10);

						if (endptr01[0] != '\0' || arg_type[1])
						{
							printf("Argument 2 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (ins_str < 1)
						{
							ins_str = 1;
						}

						if (ins_str > g_cond.str_numb)
						{
							ins_str = g_cond.str_numb;
						}

						if (strlen(arg_cage[2]) > 2)
						{
							printf("Invalid symbol notation - given more than 2 bytes. Aborted.\n");
							break;
						}

						if (!g_cond.str_numb)
						{
							char *born;

							born = (char *) malloc(CHAR_SIZE * 2);

							if (strlen(arg_cage[2]) == 1)
							{
								born[0] = arg_cage[2][0];
								born[1] = '\0';
								insrt_one(&global_text, 0,
								born, &g_cond.str_numb);
								free(born);
								g_cond.svd_stat = 0;
							}

							else
							{
								int l = CTRL_SYM_NUM - 1;
								int ins_further = 0;
								char what_ins;
								char crl_sym = arg_cage[2][1];

								if (arg_cage[2][0] != '\\')
								{
									printf("Invalid special symbol notation. Aborted.\n");
									break;
								}
	
								while (l >= 0)
								{
									if (crl_sym == 
									control_set[l])
									{
										what_ins = 7 + l;
										ins_further = 1;
									}

									l--;
								}

								if (ins_further)
								{
									born[0] = what_ins;
									born[1] = '\0';

									insrt_one(&global_text, 0,
									born, &g_cond.str_numb);
									free(born);
									adjust_lst(&global_text,
									&g_cond.str_numb);
									g_cond.svd_stat = 0;
									break;
								}

								if (crl_sym == '\\' ||
								    crl_sym == '\'' ||
								    crl_sym == '\"' ||
								    crl_sym == '\?')
								{
									born[0] = crl_sym;
									born[1] = '\0';

									insrt_one(&global_text, 0,
									born, &g_cond.str_numb);
									free(born);
									adjust_lst(&global_text,
									&g_cond.str_numb);
									g_cond.svd_stat = 0;
									break;
								}
							}

							break;
						}

						objct_num = 1;
						objct = global_text.head;

						while (objct_num != ins_str)
						{
							objct = objct -> next;
							objct_num++;
						}
						
						if (ins_elem < 1)
						{
							ins_elem = 1;
						}

						if (ins_elem > objct -> leng)
						{
							ins_elem = objct -> leng + 1;
						}

						if (strlen(arg_cage[2]) == 1)
						{
							long cc = 0;

							changed = (char *) malloc
							(CHAR_SIZE * (objct -> leng + 2));

							while (cc < (ins_elem - 1))
							{
								changed[cc] = objct -> body[cc];
								cc++;
							}

							changed[cc] = arg_cage[2][0];
							cc++;

							while (cc < (objct -> leng + 2))
							{
								changed[cc] = objct -> body[cc - 1];
								cc++;
							}
							
							free(objct -> body);
							objct -> body = changed;
							(objct -> leng)++;
							g_cond.str_numb += adjust_bckn(&global_text,
							objct);
							g_cond.svd_stat = 0;
							break;
						}

						else
						{
							if (arg_cage[2][0] != '\\')
							{
								printf("Invalid special symbol notation. Aborted.\n");
								break;
							}

							else
							{
								int l = CTRL_SYM_NUM - 1;
								int ins_further = 0;
								char what_ins;
								char crl_sym = arg_cage[2][1];
								long cc = 0;

								changed = (char *) malloc
								(CHAR_SIZE * (objct -> leng + 2));

								while (cc < (ins_elem - 1))
								{
								changed[cc] = objct -> body[cc];
									cc++;
								}
	
								while (l >= 0)
								{
									if (crl_sym == 
									control_set[l])
									{
										what_ins = 7 + l;
										ins_further = 1;
									}

									l--;
								}

								if (ins_further)
								{
									changed[cc] = what_ins;
									cc++;

									while (cc < (objct -> leng+2))
									{
									changed[cc]=objct->body[cc-1];
									cc++;
									}
							
									free(objct -> body);
									objct -> body = changed;
									(objct -> leng)++;
									g_cond.str_numb += adjust_bckn
									(&global_text, objct);
									g_cond.svd_stat = 0;
									break;
								}

								if (crl_sym == '\\' ||
								    crl_sym == '\'' ||
								    crl_sym == '\"' ||
								    crl_sym == '\?')
								{
									changed[cc] = crl_sym;
									cc++;
									while (cc < (objct -> leng+2))
									{
									changed[cc]=objct->body[cc-1];
									cc++;
									}
							
									free(objct -> body);
									objct -> body = changed;
									(objct -> leng)++;
									g_cond.str_numb += adjust_bckn
									(&global_text, objct);
									g_cond.svd_stat = 0;
									break;
								}
							}
						}

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 7:		/* replace substring */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					long range_1;
					long range_2;
					char *endptr4;

					case 4:
						range_1 = strtoul(arg_cage[0], &endptr4, 10);

						if (endptr4[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}
	
						range_2 = strtoul(arg_cage[1], &endptr4, 10);

						if (endptr4[0] != '\0' || arg_type[1])
						{
							printf("Argument 2 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (range_1 < 1 || range_2 < 1)
						{
							printf("Arguments 1, 2 can't be less than 1. Aborted.\n");
							break;
						}

						if (range_1 > range_2)
						{
							printf("Incorrect argument order. Second value must be greater or equal to the first. Aborted.\n");
							break;
						}

						if (range_1 > g_cond.str_numb)
						{
							printf("Invalid second value - bigger than general string number. Aborted.\n");
							break;
						}

						if (!arg_type[3])
						{
							printf("Argument 4 must be a string in quotes. Aborted.");
							break;
						}

						if (!arg_type[2] &&
						(!strcmp(arg_cage[2], "^") || 
						 !strcmp(arg_cage[2], "$")))
						{
							long iter = range_2 - range_1 + 1;
							long curr_numb = 1;
							struct bidir_item_lst *tmp = global_text.head;
							char *new_thing;
							int decision;

							while (curr_numb != range_1)
							{	
								curr_numb++;
								tmp = tmp -> next;
							}

							decision = (!strcmp(arg_cage[2], "^")) ?
							0 : 1;

							while (iter)
							{
								new_thing = str_concat
								(decision, tmp -> body, arg_cage[3]);
								free(tmp -> body);
								tmp -> body = new_thing;
								tmp -> leng = strlen(new_thing);
								tmp = tmp -> next;
								iter--;
							}

							strbck_lst(&global_text);
							adjust_lst(&global_text, &g_cond.str_numb);
							g_cond.svd_stat = 0;
							break;
						}

						else
						{	
							long iter = range_2 - range_1 + 1;
							long curr_numb = 1;
							long len_1, len_2;
							struct bidir_item_lst *tmp = global_text.head;

							while (curr_numb != range_1)
							{	
								curr_numb++;
								tmp = tmp -> next;
							}
							
							len_1 = strlen(arg_cage[2]);
							len_2 = strlen(arg_cage[3]);

							while (iter)
							{
								char *replaced;
								long g = 0;

								while ((replaced = str_replace
								(tmp -> body, arg_cage[2],
								arg_cage[3], len_1, len_2,
								&g)) != NULL)
								{
									free(tmp -> body);
									tmp -> body = replaced;
									tmp -> leng =
									strlen(replaced);
								}

								tmp = tmp -> next;
								iter--;
							}

							strbck_lst(&global_text);
							adjust_lst(&global_text, &g_cond.str_numb);
							g_cond.svd_stat = 0;
							break;
						}

						break;

					case 3:
						range_1 = strtoul(arg_cage[0], &endptr4, 10);

						if (endptr4[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}
	
						range_2 = g_cond.str_numb;

						if (range_1 > range_2)
						{
							printf("Invalid first value - bigger than general string number. Aborted.\n");
							break;
						}

						if (!arg_type[2])
						{
							printf("Argument 3 must be a string in quotes. Aborted.");
							break;
						}

						if (!arg_type[1] &&
						(!strcmp(arg_cage[1], "^") || 
						 !strcmp(arg_cage[1], "$")))
						{
							long iter = range_2 - range_1 + 1;
							long curr_numb = 1;
							struct bidir_item_lst *tmp = global_text.head;
							char *new_thing;
							int decision;

							while (curr_numb != range_1)
							{	
								curr_numb++;
								tmp = tmp -> next;
							}

							decision = (!strcmp(arg_cage[1], "^")) ?
							0 : 1;

							while (iter)
							{
								new_thing = str_concat
								(decision, tmp -> body, arg_cage[2]);
								free(tmp -> body);
								tmp -> body = new_thing;
								tmp -> leng = strlen(new_thing);
								tmp = tmp -> next;
								iter--;
							}

							strbck_lst(&global_text);
							adjust_lst(&global_text, &g_cond.str_numb);
							g_cond.svd_stat = 0;
							break;
						}

						else
						{	
							long iter = range_2 - range_1 + 1;
							long curr_numb = 1;
							long len_1, len_2;
							struct bidir_item_lst *tmp = global_text.head;

							while (curr_numb != range_1)
							{	
								curr_numb++;
								tmp = tmp -> next;
							}
							
							len_1 = strlen(arg_cage[1]);
							len_2 = strlen(arg_cage[2]);

							while (iter)
							{
								char *replaced;
								long point = 0;

								while ((replaced = str_replace
								(tmp -> body, arg_cage[1],
								arg_cage[2], len_1, len_2,
								&point)) != NULL)
								{
									free(tmp -> body);
									tmp -> body = replaced;
									tmp -> leng =
									strlen(replaced);
								}

								tmp = tmp -> next;
								iter--;
							}

							strbck_lst(&global_text);
							adjust_lst(&global_text, &g_cond.str_numb);
							g_cond.svd_stat = 0;
							break;
						}
						break;

					case 2:
						range_2 = g_cond.str_numb;

						if (!arg_type[1])
						{
							printf("Argument 2 must be a string in quotes. Aborted.");
							break;
						}

						if (!arg_type[0] &&
						(!strcmp(arg_cage[0], "^") || 
						 !strcmp(arg_cage[0], "$")))
						{
							long iter = range_2;	
							struct bidir_item_lst *tmp = global_text.head;
							char *new_thing;
							int decision;

							decision = (!strcmp(arg_cage[0], "^")) ?
							0 : 1;

							while (iter)
							{
								new_thing = str_concat
								(decision, tmp -> body, arg_cage[1]);
								free(tmp -> body);
								tmp -> body = new_thing;
								tmp -> leng = strlen(new_thing);
								tmp = tmp -> next;
								iter--;
							}

							strbck_lst(&global_text);
							adjust_lst(&global_text, &g_cond.str_numb);
							g_cond.svd_stat = 0;
							break;
						}

						else
						{	
							long iter = range_2;
							long len_1, len_2;
							struct bidir_item_lst *tmp = global_text.head;
							
							len_1 = strlen(arg_cage[0]);
							len_2 = strlen(arg_cage[1]);

							while (iter)
							{
								char *replaced;
								long point = 0;

								while ((replaced = str_replace
								(tmp -> body, arg_cage[0],
								arg_cage[1], len_1, len_2,
								&point)) != NULL)
								{
									free(tmp -> body);
									tmp -> body = replaced;
									tmp -> leng =
									strlen(replaced);
								}

								tmp = tmp -> next;
								iter--;
							}

							strbck_lst(&global_text);
							adjust_lst(&global_text, &g_cond.str_numb);
							g_cond.svd_stat = 0;
							break;
						}
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 8:		/* delete range */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					long fr_place1;
					long to_place1;
					long how_many1;
					char *endptr1;

					case 2:
						fr_place1 = strtoul(arg_cage[0], &endptr1, 10);

						if (endptr1[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}
	
						to_place1 = strtoul(arg_cage[1], &endptr1, 10);

						if (endptr1[0] != '\0' || arg_type[1])
						{
							printf("Argument 2 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (fr_place1 < 1 || to_place1 < 1)
						{
							printf("Arguments can't be less than 1. Aborted.\n");
							break;
						}

						if (fr_place1 > to_place1)
						{
							printf("Incorrect argument order. Second value must be greater or equal to the first. Aborted.\n");
							break;
						}

						if (to_place1 > g_cond.str_numb)
						{
							printf("Invalid second value - bigger than general string number. Aborted.\n");
							break;
						}

						how_many1 = to_place1 - fr_place1 + 1;
						delete_times_list(&global_text, fr_place1, how_many1);
						g_cond.str_numb -= how_many1;
						g_cond.svd_stat = 0;
						break;

					case 1:
						fr_place1 = strtoul(arg_cage[0], &endptr1, 10);

						if (endptr1[0] != '\0' || arg_type[0])
						{
							printf("Argument must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (fr_place1 < 1)
						{
							printf("Argument can't be less than 1. Aborted.\n");
							break;
						}

						if (fr_place1 > g_cond.str_numb)
						{
							printf("Invalid value - bigger than general string number. Aborted.\n");
							break;
						}
						
						how_many1 = g_cond.str_numb - fr_place1 + 1;
						delete_times_list(&global_text, fr_place1, how_many1);
						g_cond.str_numb -= how_many1;
						g_cond.svd_stat = 0;
						break;

					default:
						ERR_INV_ARG_NUM;
						break;					
				}

				break;

			case 9:		/* delete braces */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					long border_1;
					long border_2;
					char *endptr_br;

					case 0:
						if (!g_cond.str_numb)	/* empty */
						{
							break;
						}

						delete_br(&global_text, 1, g_cond.str_numb,
						&g_cond.str_numb);
						g_cond.svd_stat = 0;
						break;

					case 1:
						border_1 = strtoul(arg_cage[0], &endptr_br, 10);

						if (endptr_br[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (border_1 > g_cond.str_numb)
						{
							printf("Invalid first value - bigger than general string number. Aborted.\n");
							break;
						}

						delete_br(&global_text, border_1, g_cond.str_numb,
						&g_cond.str_numb);
						g_cond.svd_stat = 0;
						break;

					case 2:
						border_1 = strtoul(arg_cage[0], &endptr_br, 10);

						if (endptr_br[0] != '\0' || arg_type[0])
						{
							printf("Argument 1 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						border_2 = strtoul(arg_cage[1], &endptr_br, 10);

						if (endptr_br[0] != '\0' || arg_type[1])
						{
							printf("Argument 2 must be a digital value and without quotes. Aborted.\n");
							break;
						}

						if (border_1 < 1 || border_2 < 1)
						{
							printf("Arguments can't be less than 1. Aborted.\n");
							break;
						}

						if (border_1 > border_2)
						{
							printf("Incorrect argument order. Second value must be greater or equal to the first. Aborted.\n");
							break;
						}

						if (border_2 > g_cond.str_numb)
						{
							printf("Invalid first value - bigger than general string number. Aborted.\n");
							break;
						}

						delete_br(&global_text, border_1, border_2,
						&g_cond.str_numb);
						g_cond.svd_stat = 0;
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 10:	/* exit */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					case 0:
						if (!g_cond.svd_stat)
						{
							printf("The file isn't saved. Aborted.\n");
							break;
						}

						else
						{
							if (g_cond.str_numb)
								bidir_lst_erase(&global_text);

							if (entered_copy -> head != NULL)
								bidir_lst_erase(entered_copy);

							if (g_cond.name != NULL &&
							g_cond.name != argv[1])
								free(g_cond.name);

							erase_cage(arg_cage, arg_type);
							return 0;
						}

					case 1:
						if (!strcmp(arg_cage[0], "force") &&
						arg_type[0] == 0)
						{
							if (g_cond.str_numb)
								bidir_lst_erase(&global_text);

							if (entered_copy -> head != NULL)
								bidir_lst_erase(entered_copy);

							if (g_cond.name != NULL &&
							g_cond.name != argv[1])
								free(g_cond.name);

							erase_cage(arg_cage, arg_type);
							return 0;
						}

						else
						{
							printf("Invalid argument.\n");
							break;
						}

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 11:	/* read */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					case 1:
						if (global_text.head != NULL)
						{
							bidir_lst_erase(&global_text);
						}

						if (!arg_type[0])
						{
							printf("Name should be in quotes. Aborted.\n");
							break;
						}

						g_cond.str_numb = 0;
						aux_str_1 = str_bck_mod(arg_cage[0]);
						filename_1 = fopen(aux_str_1, "r");
						printf("File for read: <%s>\n", aux_str_1);
						free(aux_str_1);

						if (alert_open_file(filename_1))
							break;

						g_cond.opn_stat = 1;
						read_to_list(filename_1, &global_text,
						&g_cond.str_numb);	

						if (alert_close_file(fclose(filename_1)))
							break;

						g_cond.opn_stat = 0;
						g_cond.svd_stat = 1;
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 12:	/* open */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					case 1:
						if (global_text.head != NULL)
						{
							bidir_lst_erase(&global_text);
						}

						if (!arg_type[0])
						{
							printf("Name should be in quotes. Aborted.\n");
							break;
						}

						g_cond.str_numb = 0;
						aux_str_1 = str_bck_mod(arg_cage[0]);
						filename_1 = fopen(aux_str_1, "r");
						printf("File for open: <%s>\n", aux_str_1);

						if (alert_open_file(filename_1))
							break;

						g_cond.opn_stat = 1;
						read_to_list(filename_1, &global_text,
						&g_cond.str_numb);	

						if (alert_close_file(fclose(filename_1)))
							break;

						g_cond.opn_stat = 0;

						if (g_cond.name != NULL)
							free(g_cond.name);

						g_cond.name = aux_str_1;
						g_cond.ass_stat = 1;
						g_cond.svd_stat = 1;	
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 13:	/* write */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					case 0:
						if (!g_cond.ass_stat)
						{
							printf("No such association. Aborted.\n");
							break;
						}

						else
						{
							filename_1 = fopen(g_cond.name, "w");
							printf("File for write: <%s>\n", g_cond.name);

							if (alert_open_file(filename_1))
								break;

							g_cond.opn_stat = 1;
							write_from_list(filename_1, &global_text);

							if (alert_close_file(fclose(filename_1)))
								break;

							g_cond.opn_stat = 0;
							g_cond.svd_stat = 1;
						}

						break;

					case 1:

						if (!arg_type[0])
						{
							printf("Name should be in quotes. Aborted.\n");
							break;
						}

						aux_str_1 = str_bck_mod(arg_cage[0]);
						filename_1 = fopen(aux_str_1, "w");
						printf("File for write: <%s>\n", aux_str_1);	

						if (alert_open_file(filename_1))
							break;

						g_cond.opn_stat = 1;
						write_from_list(filename_1, &global_text);

						if (alert_close_file(fclose(filename_1)))
							break;

						g_cond.opn_stat = 0;
						g_cond.svd_stat = 1;

						if (!g_cond.ass_stat)
						{
							if (g_cond.name != NULL)
								free(g_cond.name);

							g_cond.name = aux_str_1;
							g_cond.ass_stat = 1;
							break;
						}

						free(aux_str_1);
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}
	
				break;

			case 14:	/* set name */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);
				
				aux_str_1 = NULL;

				switch (aux_int_arg)
				{
					case 1:
						if (cmd_place[9] == '\"' && cmd_place[10] == '\"' &&
							cmd_place[11] == '\0')
						{
							if (g_cond.name != NULL)
								free(g_cond.name);

							g_cond.name = NULL;
							g_cond.ass_stat = 0;
							printf("Assotiation has deleted.\n");
							break;
						}

						if (!arg_type[0])
						{
							printf("Name should be in quotes. Aborted.\n");
							break;
						}

						aux_str_1 = str_bck_mod(arg_cage[0]);
						printf("Associated: <%s>\n", aux_str_1);

						if (g_cond.name != NULL)
							free(g_cond.name);

						g_cond.name = aux_str_1;
						g_cond.ass_stat = 1;
						g_cond.svd_stat = 0;	
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 15:	/* help */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					struct termios old_attr, new_attr;
					int sym_c;
					long str_diap;
					long sym_diap;
					long help_len;

					case 0:
						filename_1 = fopen("helpc.txt", "r");

						if (alert_open_file(filename_1))
							goto ForceContinue;

						help_len = 0;
						read_to_list(filename_1, &help_box,
						&help_len);

						printf("||%ld||\n", help_len);

						if (alert_close_file(fclose(filename_1)))
							goto ForceContinue;

						str_diap = sym_diap = 0;

						if (!isatty(0))
						{
							printf("Input is not from terminal.");
							return 4;
						}

						if (!is_terminal_out)
						{
							while (str_diap != (g_cond.str_numb / height))
							{
								print_diap_no(&help_box, str_diap,
								sym_diap, width, height, 0,
								help_len);
								str_diap++;
							}
						}

						else
						{

						tcgetattr(0, &old_attr);
						memcpy(&new_attr, &old_attr, sizeof(struct termios));
						new_attr.c_lflag &= ~ECHO;
						new_attr.c_lflag &= ~ICANON;
						new_attr.c_cc[VMIN] = 1;
						tcsetattr(0, TCSANOW, &new_attr);
						sym_c = '\0';

						while (sym_c != 'q')
						{
							int pid_c = -1;
							int clear_status;

							pid_c = fork();

        		        	                if (pid_c == -1)
	       	                        		{
        	                                		perror("Unsuccessful clear.\n");
	                	                        	break;
        	                	        	}

                	                		if (!pid_c)
                        	        		{
                                	        		execlp("clear", "clear", NULL);
                                        			perror("Entered but failed clear.\n");
                                        			break;
	                                		}

        	                        		wait(&clear_status);	

							if (sym_c == -1)	/* catching ^C */
								break;

							if (sym_c == ',')
							{
								if (sym_diap != 0)
									sym_diap--;
							}

							if (sym_c == '.')
								sym_diap++;

							if (sym_c == ' ')
							{
								if (str_diap ==
								(help_len / height))
									break;

								str_diap++;
							}
						
							if (sym_c == 'b')
							{
								if (str_diap != 0)
									str_diap--;
							}

							print_diap_no(&help_box, str_diap,
							sym_diap, width, height, 0, help_len);
							printf("\x1b[31;1m[Space B < > Q]\x1b[37m");

							if (sym_c == '\004')
								break;

							sym_c = getchar();
						}

						tcsetattr(0, TCSANOW, &old_attr);

						}

						bidir_lst_erase(&help_box);
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 16:	/* directory */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					case 0:
						aux_int = fork();

						if (aux_int == -1)
						{
							perror("Unsuccessful ls.\n");
							break;
						}

						if (!aux_int)
						{
							execlp("ls", "ls", "-l", NULL);
							perror("Entered but failed ls.\n");
							break;
						}
				
						wait(&ls_status);
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}

				break;

			case 17:	/* status */

				aux_int_arg = split_ord(cmd_place+strlen
				(command_database[cmd_cursor]), arg_cage, arg_type);

				switch (aux_int_arg)
				{
					case 0:
						if (g_cond.ass_stat)
						{
							printf("Associated status = %d;\n",
							g_cond.ass_stat);
							printf("Written status = %d;\n",
							g_cond.svd_stat);
							printf("Associated name = %s;\n",
							g_cond.name);
							printf("String number = %ld;\n",
							g_cond.str_numb);
							break;
						}

						printf("Associated status = %d;\n",
						g_cond.ass_stat);
						printf("Written status = %d;\n",
						g_cond.svd_stat);
						printf("String number = %ld;\n",
						g_cond.str_numb);	
						break;

					default:
						ERR_INV_ARG_NUM;
						break;
				}
	
				break;

			default:
				printf("Undefined command at the moment.\n");
		}

		ForceContinue: 
		erase_cage(arg_cage, arg_type);

		if (g_cond.ent_del)
			bidir_lst_erase(&entered_str);

		g_cond.ent_del = 1;
		continue;
	}
		
	} /* external while */

	if (g_cond.str_numb)
		bidir_lst_erase(&global_text);

	if (entered_copy -> head != NULL)
		bidir_lst_erase(entered_copy);

	if (g_cond.name != NULL && g_cond.name != argv[1])
		free(g_cond.name);

	return 0;
}
