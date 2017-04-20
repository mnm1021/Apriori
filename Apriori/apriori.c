#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/*
* definition of linked list.
* this linked list can be used for 2 ways :
*    1, represent 'transaction'.
*    2, store transactions from file.
*/
typedef struct list_node
{
	union
	{
		struct list_node *transaction;
		struct list_node *set;
		int item_id;
	};
	int set_count; /* only used in 'set' node. */
	int size_of_set; /* only used in 'subsets'. */
	struct list_node *next_node;
} list_node;

double round_(double x, int digit)
{
	return (floor((x)*pow(10.0, digit) + 0.5f) / pow(10.0, digit));
}

/* list utilities. */
list_node* create_node(int item_id);
void delete_list(list_node *head);
int delete_set_from_list(list_node* list, list_node* set);
void delete_list_container(list_node *head);
list_node* get_subset(list_node* list);


/* input, output functions. */
list_node* read_transactions(int* num_trans, list_node* sets, FILE* fp);


/* apriori - check if set is in list. */
int is_set_in_list(list_node* list, list_node* set, int size);
void add_set_data(list_node* sets, list_node* new_set);
list_node* prune_sets(list_node* sets, int min_support, int num_trans);
void print_subsets(list_node* curr_subset, list_node* transaction_list, int num_trans, FILE* fp);


int main(int argc, char **argv)
{
	int min_support, num_trans = 0;
	FILE *fp_input, *fp_output;
	list_node *transaction_list, *curr_trans;
	list_node *sets, *subsets;
	list_node *start_subset, *end_subset, *curr_subset, *tmp;

	/* check argc. */
	if (argc != 4)
	{
		printf("Usage : ./apriori [min_support] [input_file] [output_file]\n");
		exit(-1);
	}

	/* initialize min_support, fp's, sets. */
	min_support = atoi(argv[1]);
	fopen_s(&fp_input, argv[2], "rt");
	fopen_s(&fp_output, argv[3], "wt");
	sets = (list_node*)malloc(sizeof(list_node));
	memset(sets, 0, sizeof(list_node));

	/* read transactions from file, get all subsets from sets. */
	transaction_list = read_transactions(&num_trans, sets, fp_input);
	sets = prune_sets(sets, min_support, num_trans);
	subsets = get_subset(sets);

	if (sets != NULL)
		delete_list_container(sets);

	/* loop until there is no more appropriate subset. */
	for (start_subset = subsets;
	     start_subset != NULL;
		 start_subset = end_subset->next_node)
	{
		/* find the end of current size. */
		end_subset = start_subset;
		while (end_subset->next_node != NULL &&
		       end_subset->size_of_set == end_subset->next_node->size_of_set)
			end_subset = end_subset->next_node;

		/* if size is not 1, scan through transactions, and prune. */
		if (start_subset->size_of_set != 1)
		{
			for (curr_subset = start_subset;
			     curr_subset != end_subset->next_node;
				 /* EMPTY */)
			{
				/* count curr_trans through transactions. */
				for (curr_trans = transaction_list;
				     curr_trans != NULL;
				 	 curr_trans = curr_trans->next_node)
				{
					if (is_set_in_list(curr_trans->transaction,
						curr_subset->set, curr_subset->size_of_set))
					{
						curr_subset->set_count += 1;
					}
				}

				/* if count is less than min_support, prune elements. */
				if ((double)curr_subset->set_count / (double)num_trans * 100 < min_support)
				{
					/* if only one left, break loop. */
					if (start_subset == end_subset)
						goto LOOP_BREAK;

					tmp = curr_subset->next_node;

					/* if start_subset is to be deleted, set new start_subset. */
					if (start_subset == curr_subset)
						start_subset = curr_subset->next_node;

					if (delete_set_from_list(subsets, curr_subset) == 0)
					{
						subsets = tmp;
					}

					curr_subset = tmp;
				}
				else
				{
					/* print association of current subset. */
					print_subsets(curr_subset, transaction_list, num_trans, fp_output);

					/* advance. */
					curr_subset = curr_subset->next_node;
				}
			}
		}
	}

LOOP_BREAK:

	/* free all lists. */
	if (transaction_list != NULL)
		delete_list_container(transaction_list);
	if (subsets != NULL)
		delete_list_container(subsets);

	/* close fp's. */
	fclose(fp_input);
	fclose(fp_output);

	system("PAUSE");

	return 0;
}


/*
* read from file, store and return whole transactions.
*/
list_node* read_transactions(int* num_trans, list_node* sets, FILE* fp)
{
	int id;
	char buf[100], *token, *saveptr;
	list_node *head, *curr_transaction, *curr_trans_node, *new_node;

	head = (list_node*)malloc(sizeof(list_node));
	memset(head, 0, sizeof(list_node));
	curr_transaction = head;

	while (feof(fp) == 0)
	{
		/* add transaction's count. */
		(*num_trans) += 1;
		/* read one line(one transaction) to buffer. */
		fgets(buf, 100, fp);

		curr_trans_node = curr_transaction;

		/* get each item id from a transaction. */
		for (token = strtok_s(buf, "\t\n ", &saveptr);
		     token != NULL;
			 token = strtok_s(NULL, "\t\n ", &saveptr))
		{
			if (token[0] != '\n')
			{
				id = atoi(token);
				new_node = create_node(id);

				/* create new node for item id. */
				if (curr_trans_node == curr_transaction)
				{
					/* if first node for this transaction, create new 'transaction'. */
					curr_trans_node->transaction = new_node;
					curr_trans_node = curr_trans_node->transaction;
				}
				else
				{
					/* if second or more node, create next node. */
					curr_trans_node->next_node = new_node;
					curr_trans_node = curr_trans_node->next_node;
				}

				/* add data to item set. */
				add_set_data(sets, new_node);
			}
		}

		/* allocate next node if next is avaliable. */
		if (feof(fp) == 0)
		{
			curr_transaction->next_node = (list_node*)malloc(sizeof(list_node));
			curr_transaction = curr_transaction->next_node;
			memset(curr_transaction, 0, sizeof(list_node));
		}
	}

	return head;
}


/*
* return every subsets of given list.
*/
list_node* get_subset(list_node* list)
{
	int size_of_set;
	list_node *i_list;
	list_node *ret_list, *ret_list_end, *ret_list_tmp, *ret_list_curr_set;
	list_node *new_list, *new_list_end, *new_list_curr_set;
	list_node *tmp;

	if (list == NULL)
		return NULL;

	/* initialize list, list descriptor. */
	i_list = list;
	ret_list = (list_node*)malloc(sizeof(list_node));
	memset(ret_list, 0, sizeof(list_node));
	ret_list_end = ret_list;

	/* set first node. */
	ret_list_end->set = create_node(i_list->set->item_id);
	ret_list_end->size_of_set = 1;
	ret_list_end->set_count = i_list->set_count;

	i_list = i_list->next_node;

	/* sweep through all list, add every element. */
	while (i_list != NULL)
	{
		/* create new list. */
		new_list = (list_node*)malloc(sizeof(list_node));
		memset(new_list, 0, sizeof(list_node));

		/* set list descriptor. */
		new_list_end = new_list;

		/* set first node : new element. */
		new_list_end->set = create_node(i_list->set->item_id);
		new_list_end->size_of_set = 1;
		new_list_end->set_count = i_list->set_count;

		/* sweep through all nodes of ret_list. copy all elements to new_list. */
		for (ret_list_tmp = ret_list;
		     ret_list_tmp != NULL;
		 	 ret_list_tmp = ret_list_tmp->next_node)
		{
			/* create new node for new_list. */
			new_list_end->next_node = (list_node*)malloc(sizeof(list_node));
			new_list_end = new_list_end->next_node;
			memset(new_list_end, 0, sizeof(list_node));

			/* set first set's node : i_list's element. */
			new_list_end->set = create_node(i_list->set->item_id);

			/* initialize variables. */
			size_of_set = 1;
			new_list_curr_set = new_list_end->set;

			/* sweep through all nodes of ret_list_tmp's set. copy each nodes to new_list_end. */
			for (ret_list_curr_set = ret_list_tmp->set;
			ret_list_curr_set != NULL;
				ret_list_curr_set = ret_list_curr_set->next_node)
			{
				size_of_set++;
				new_list_curr_set->next_node = create_node(ret_list_curr_set->item_id);
				new_list_curr_set = new_list_curr_set->next_node;
			}
			new_list_end->size_of_set = size_of_set;
		}

		/* add new_list to ret_list, sort in order. */
		for (ret_list_tmp = ret_list;
		     ret_list_tmp != NULL;
			 ret_list_tmp = tmp)
		{
			/* find the last node of size N in ret_list. */
			ret_list_end = ret_list_tmp;
			while (ret_list_end->next_node != NULL &&
				  ret_list_end->next_node->size_of_set == size_of_set)
				  ret_list_end = ret_list_end->next_node;

			/* find the last node of size N in new_list. */
			size_of_set = new_list->size_of_set;
			new_list_end = new_list;
			while (new_list_end->next_node != NULL &&
				  new_list_end->next_node->size_of_set == size_of_set)
				  new_list_end = new_list_end->next_node;

			/* link nodes. */
			tmp = ret_list_end->next_node;
			ret_list_end->next_node = new_list;
			new_list = new_list_end->next_node;
			new_list_end->next_node = tmp;
		}

		/* add the last node. */
		new_list_end->next_node = new_list;

		/* advance. */
		i_list = i_list->next_node;
	}

	return ret_list;
}


/*
* prune element which has less than min_support.
*/
list_node* prune_sets(list_node* sets, int min_support, int num_trans)
{
	list_node *curr_set, *next_set;

	curr_set = sets;
	while (curr_set != NULL)
	{
		next_set = curr_set->next_node;
		if ((double)curr_set->set_count / (double)num_trans * 100 < min_support)
		{
			if (delete_set_from_list(sets, curr_set) == 0)
			{
				sets = next_set;
				delete_list(curr_set->set);
				free(curr_set);
			}
		}
		curr_set = next_set;
	}

	return sets;
}

/*
* check if 'set' is in list.
*/
int is_set_in_list(list_node* list, list_node* set, int size)
{
	int* flags;
	int ret = 0, index;
	list_node *tmp_trans, *tmp_set;

	/* set flags. */
	flags = (int*)malloc(sizeof(int)*size);
	memset(flags, 0, sizeof(int)*size);

	/* search through a list, set flags. */
	tmp_trans = list;
	while (tmp_trans != NULL)
	{
		/* initialize. */
		tmp_set = set;
		index = 0;

		/* search through set, if item_id is same as set's id, set flag and break. */
		while (tmp_set != NULL)
		{
			if (tmp_trans->item_id == tmp_set->item_id)
			{
				flags[index] = 1;
				break;
			}

			/* advance. */
			tmp_set = tmp_set->next_node;
			index++;
		}

		/* advance. */
		tmp_trans = tmp_trans->next_node;
	}

	/* get all flags. */
	for (index = 0; index<size; index++)
		ret += flags[index];

	free(flags);

	return ret == size;
}


/*
* create new list node.
*/
list_node* create_node(int item_id)
{
	list_node *ret;

	ret = (list_node*)malloc(sizeof(list_node));
	memset(ret, 0, sizeof(list_node));
	ret->item_id = item_id;

	return ret;
}


/*
* delete a node from 'set'. if head, return 0. else, return 1.
*/
int delete_set_from_list(list_node* list, list_node* set)
{
	list_node* prev;

	if (list == set)
		return 0;

	prev = list;

	while (prev != NULL)
	{
		if (prev->next_node == set)
		{
			prev->next_node = set->next_node;
			return 1;
		}

		prev = prev->next_node;
	}

	return -1; /* if reached this code, something should be wrong. */
}


/*
* delete whole linked list.
*/
void delete_list(list_node *head)
{
	list_node *tmp;

	while (head != NULL)
	{
		tmp = head->next_node;
		free(head);
		head = tmp;
	}
}


/*
* delete list which contains more data.
*/
void delete_list_container(list_node *head)
{
	list_node* tmp;

	while (head != NULL)
	{
		delete_list(head->transaction);
		tmp = head->next_node;
		free(head);
		head = tmp;
	}
}


/*
* add a new set to 'sets'.
*/
void add_set_data(list_node* sets, list_node* new_set)
{
	list_node *curr_set;
	list_node *tmp_new_set, *tmp_curr_set;

	/* add data to item set. */
	curr_set = sets;
	while (curr_set != NULL)
	{
		/* if initial state, create new 'set' to tmp_sets and break loop. */
		if (curr_set->set == NULL)
		{
			/* create new set. */
			curr_set->set = create_node(new_set->item_id);
			tmp_curr_set = curr_set->set;
			tmp_new_set = new_set->next_node;
			while (tmp_new_set != NULL)
			{
				tmp_curr_set->next_node = create_node(tmp_new_set->item_id);
				tmp_curr_set = tmp_curr_set->next_node;
				tmp_new_set = tmp_new_set->next_node;
			}

			/* set count. */
			curr_set->set_count = 1;
			break;
		}

		/* if set is already in list, add list's set count and break loop. */
		if (is_set_in_list(curr_set->set, new_set, 1))
		{
			curr_set->set_count += 1;
			break;
		}

		/* if there is no more node, add a node. */
		if (curr_set->next_node == NULL)
		{
			/* create new node. */
			curr_set->next_node = (list_node*)malloc(sizeof(list_node));
			curr_set = curr_set->next_node;
			memset(curr_set, 0, sizeof(list_node));

			/* create new set. */
			curr_set->set = create_node(new_set->item_id);
			tmp_curr_set = curr_set->set;
			tmp_new_set = new_set->next_node;
			while (tmp_new_set != NULL)
			{
				tmp_curr_set->next_node = create_node(tmp_new_set->item_id);
				tmp_curr_set = tmp_curr_set->next_node;
				tmp_new_set = tmp_new_set->next_node;
			}

			/* set count. */
			curr_set->set_count = 1;
		}

		/* advance. */
		curr_set = curr_set->next_node;
	}
}

void print_subsets(list_node* curr_subset, list_node* transaction_list, int num_trans, FILE* fp)
{
	list_node *curr_tmp_set, *c;
	list_node *tmp_, *new_subset;
	list_node *n1, *n2, *n1_tmp, *n2_tmp, *tmp__;
	list_node *curr_trans;
	int cnt_n1, cnt_n2;

	curr_tmp_set = malloc(sizeof(list_node));
	memset(curr_tmp_set, 0, sizeof(list_node));
	curr_tmp_set->set = create_node(curr_subset->set->item_id);
	c = curr_tmp_set;
	for (tmp_ = curr_subset->set->next_node; tmp_ != NULL; tmp_ = tmp_->next_node)
	{
		curr_tmp_set->next_node = malloc(sizeof(list_node));
		curr_tmp_set = curr_tmp_set->next_node;
		memset(curr_tmp_set, 0, sizeof(list_node));
		curr_tmp_set->set = create_node(tmp_->item_id);
	}
	
	new_subset = get_subset(c);

	for (tmp_ = new_subset; tmp_->next_node != NULL; tmp_ = tmp_->next_node);
	n2 = tmp_->set;
	
	for (tmp_ = new_subset; tmp_->next_node != NULL; tmp_ = tmp_->next_node)
	{
		n1 = tmp_->set;
		for (n1_tmp = n1; n1_tmp != NULL; n1_tmp = n1_tmp->next_node)
		{
			if (n1_tmp->item_id == n2->item_id)
			{
				tmp__ = n2;
				n2 = n2->next_node;
				free(tmp__);
			}
			else
				for (n2_tmp = n2; n2_tmp != NULL && n2_tmp->next_node != NULL; n2_tmp = n2_tmp->next_node)
					if (n1_tmp->item_id == n2_tmp->next_node->item_id)
					{
						tmp__ = n2_tmp->next_node;
						n2_tmp->next_node = tmp__->next_node;
						free(tmp__);
					}
		}

		cnt_n1 = cnt_n2 = 1;

		fprintf(fp, "{%d", n1->item_id);
		for (n1_tmp = n1->next_node; n1_tmp != NULL; n1_tmp = n1_tmp->next_node)
		{
			fprintf(fp, ",%d", n1_tmp->item_id);
			cnt_n1++;
		}
		fprintf(fp, "}\t");

		fprintf(fp, "{%d", n2->item_id);
		for (n2_tmp = n2->next_node; n2_tmp != NULL; n2_tmp = n2_tmp->next_node)
		{
			fprintf(fp, ",%d", n2_tmp->item_id);
			cnt_n2++;
		}
		fprintf(fp, "}\t");

		/* count curr_trans through transactions. */
		double support=0, confidence=0, n1_total=0;
		for (curr_trans = transaction_list;
		     curr_trans != NULL;
			 curr_trans = curr_trans->next_node)
		{
			int is_n1, is_n2;
			is_n1 = is_set_in_list(curr_trans->transaction, n1, cnt_n1);
			is_n2 = is_set_in_list(curr_trans->transaction, n2, cnt_n2);

			if (is_n1 && is_n2)
				support += 1;
			if (is_n1)
			{
				n1_total += 1;
				if (is_n2)
					confidence += 1;
			}
		}
		support = support / (double)num_trans * 100;
		confidence = confidence / n1_total * 100;
		support = round_(support, 2);
		confidence = round_(confidence, 2);
		fprintf(fp, "%.2lf\t%.2lf\n", support, confidence);

		for (n2_tmp = n2; n2_tmp->next_node != NULL; n2_tmp = n2_tmp->next_node);
		for (n1_tmp = n1; n1_tmp != NULL; n1_tmp = n1_tmp->next_node)
		{
			n2_tmp->next_node = create_node(n1_tmp->item_id);
			n2_tmp = n2_tmp->next_node;
		}
	}

	delete_list_container(c);
}