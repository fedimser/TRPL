// Graph Builder 1.0 by Dmytry Fedoryaka
// GNU public license

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <ctype.h>
#include <malloc.h> 
using namespace std;

char out_file[100], in_file[100];
char reg_exp[1000], reg_exp_rpn[1000];
const int node_distance_mm = 10;
FILE *fl;

char need_full_tex =1;
 

char NULL_CHAR = 0;
const char str_e[] = "$\\epsilon$";
const char tex_preamble[] =	 
"\\documentclass[a4paper, 12pt]{article}\n"

"\\usepackage[version=0.96]{pgf}\n"
"\\usepackage{tikz}\n"
"\\usetikzlibrary{arrows,shapes,snakes,automata,backgrounds,petri}\n" 
"\\tikzstyle {nt}=[rectangle,minimum size=2mm, very thick,draw=black!50, top color=white,bottom color=black!20,font=\\ttfamily]\n"
"\\tikzstyle {t}=[rectangle,	minimum size=2mm,very thick,draw=red!50!black!50,  top color=white, 	bottom color=red!50!black!20, font=\\itshape	]\n"
"\\tikzstyle {transparent}=[rectangle, minimum size=0mm,	very thick,			draw=white, 			top color=white, 			bottom color=white	]\n"
"\\begin{document} \n";
			
			  

int COUNTER = 0;

struct node;
struct edge;
struct FA;


struct edge
{
  int id;
  node *from;
  node *to;
  char *name;
  edge* next;
  char back;
  
  edge()
  {
	back=0; 
  }
};

struct node
{

  int id;
  char *name;
  char terminal;
  char used;
  edge*  e;
  char indeg,outdeg;


  node()
  {
		id=COUNTER++;
		e = NULL;
		name = &NULL_CHAR;
		used=0;
		terminal = 0;
		indeg=outdeg=0;	
  }

  void out_children ()
  {

    int last_node = id;
	
    for (edge* it = e; it != NULL; it=it->next)
    {
	  //printf("OC %d \n", id);	
		
      node *nd = (it)->to;
      if (!nd->used)
      {
        fprintf (fl, "\\node (q%d)[%s ,%s of=q%d] {%s};\n", nd->id,
                 (nd->terminal ? "t" : "nt"),
                 (last_node == id) ? "right" : "below", last_node, name);
        last_node = nd->id;
      }
      fprintf (fl, "\\path (q%d) edge[%s%s] node[above]{%s} (q%d);\n", id,
               (nd->id == id) ? "loop above" : "->", (it->back)? ",bend right":"" ,it->name, nd->id);
      
      if(!nd->used)
      { 
		nd->used = 1; 
		nd->out_children();
      }
    }
    
  }

  void add_edge (node * to, const char *_name, char _back)
  {
    edge* new_edge = (edge*)malloc(sizeof(edge));
    new_edge->id = COUNTER++;
    new_edge->from = (this);
    new_edge->to = to;
    new_edge->name = (char *) malloc (strlen (_name));
    strcpy (new_edge->name, _name);
	
	new_edge->back=_back;
	this->outdeg++;
	to->indeg++;
	
	new_edge->next=NULL;
	
	if(e==NULL)
	{
		e=new_edge;
	}
	else
	{
		edge* it=e;
		while(it!=NULL && it->next != NULL) it=it->next;
		it->next=new_edge;
	}
  }
  
};

#define NMAX 1000
node nodes[NMAX];
int nctr=0;

struct FA
{
  node *start;
  node *fin;
};

FA *elementary_FA (char x)
{
  node* n1 =  nodes+(nctr++);
  node* n2 =  nodes+(nctr++);
  
  *n1 = node();
  *n2 = node();
  
  char name[2];
  name[0] = x;
  name[1] = 0;

  n1->add_edge (n2, name,0); 
	
  FA* fa = (FA *) malloc (sizeof (FA));
  fa->start=n1;
  fa->fin=n2;  
  
 
  

  return fa;
}
 
FA *concat_FA (FA * left, FA * right)
{
  node *n1 = left->fin;
  node *n2 = right->start;

  if(n1->e == NULL)
  {
	  n1->e=n2->e;
  }
  else
  {
	edge* te=n2->e;  
	n2->e=n1->e;
	edge* it = n2->e;
	while(it->next!=NULL) it=it->next;
	it->next=te;
  }
  
  //free (n2);

  left->fin = right->fin;
  free (right);
  return left;
}

FA *parallel_FA (FA * a1, FA * a2)
{
  node *n1 = nodes+(nctr++);
  node *n2 = nodes+(nctr++);
 
  (*n1) = node();
  (*n2) = node();

  n1->add_edge (a1->start, str_e,0);
  n1->add_edge (a2->start, str_e,0);

  a1->fin->add_edge (n2, str_e,0);
  a2->fin->add_edge (n2, str_e,0);

  a1->start = n1;
  a1->fin = n2;

  free (a2);

  return a1;
}
 
 
FA *iteration_FA (FA * a)
{
  node *n1 = nodes+(nctr++);
  node *n2 = nodes+(nctr++);

  (*n1) = node();
  (*n2) = node();


  n1->add_edge (a->start, str_e,1);
  a->fin->add_edge (n2, str_e,1);

  a->fin->add_edge (a->start, str_e,1);
  n1->add_edge(n2,str_e,1);

  a->start = n1;
  a->fin = n2;

  return a;
}

char isletter(char x)
{
	return (x>='a' && x<='z');
}

void rexp_to_rpn ()
{
  char stack[1000];

  char *p = reg_exp_rpn;
  char *sp = stack;

  for (int i = 0; reg_exp[i] != 0; i++)
  {
    char c = reg_exp[i];

    bool needcat = 0;
    
    //if( reg_exp[i]==')'  ) needcat=0;
    
    if ( (isletter (c) || c=='(') && i != 0
        && (isletter (reg_exp[i - 1]) || reg_exp[i - 1] == ')'
            || reg_exp[i - 1] == '*'))
      needcat = 1;
       
      
    if(c=='(' && i!=0 && isletter(reg_exp[i-1]) ) needcat=1;  

    if (needcat)
    {
      while (sp != stack && *(sp - 1) != '(' && *(sp - 1) != '@')
      {
        sp--;
        *p = *sp;
        p++;
      }
      *sp = '@';
      sp++;
    }

    if (isletter (c))
    {
      *p = c;
      p++;
    }
    else if (c == '(')
    {
      *sp = c;
      sp++;
    }
    else if (c == ')')
    {
      while (sp != stack && *(sp - 1) != '(')
      {
        sp--;
        *p = *sp;
        p++;
      }
      if (sp == stack)
      {
        printf ("ERROR!\n");
        exit (-1);
      }

      sp--;
      
    }
    else if (c == '+')
    {
      while (sp!=stack && *(sp - 1) != '(')
      {
        sp--;
        *p = *sp;
        p++;
      }
      *sp = '+';
      sp++;
    }
    else if (c == '*')
    {
      *p = '*';
      p++;
    }
	
	*sp=0;
   // printf ("%c %s %s %s\n", c,reg_exp, reg_exp_rpn, stack);
  } 
  while (sp != stack)
  {
    sp--;
    *p = *sp;
    p++;
  }

 // printf (" %s %s\n", reg_exp_rpn, stack);

  *p = 0; 
}

void read_reg_exp ()
{
  fl = fopen (in_file, "r");
  fscanf (fl, "%s", reg_exp);
  fclose (fl);
}

void out_graph (node * g)
{
  fl = fopen (out_file, "w");
  
  if(need_full_tex)
  {
	 fprintf(fl, "%s", tex_preamble);  
  }
  

  fprintf (fl, "\\begin{tikzpicture}\n");
  fprintf (fl, "[node distance=%dmm,>=stealth',bend angle=45,auto]\n",
           node_distance_mm);

  fprintf (fl, "\\node (init)[transparent] {};\n");
  fprintf (fl, "\\node (q%d)[%s ,right of=init] {%s};\n", g->id,
           (g->terminal ? "t" : "nt"), (g->name));
  fprintf (fl, "\\path (init) edge[->] (q%d);\n", g->id);
               
  g->used = true;

  g->out_children ();

  fprintf (fl, "\\end{tikzpicture}\n");

   if(need_full_tex)
  {
	 fprintf(fl, "\\end{document}");  
  }
  

  fclose (fl);
}
 
FA *build_NFA_rexp ()
{
  FA *stack[1000];
  FA **sp = stack;

  for (int i = 0; reg_exp_rpn[i] != 0; i++)
  {
    char c = reg_exp_rpn[i];
    if (isalpha (c))
    {
      *sp = elementary_FA (c);
      sp++;
    }
    else if (c == '@')
    {
      *(sp - 2) = concat_FA (*(sp - 2), *(sp - 1));
      sp--;
    }
    else if (c == '+')
    {
      *(sp - 2) = parallel_FA (*(sp - 2), *(sp - 1));
      sp--;
    }
    else if (c == '*')
    {
      *(sp - 1) = iteration_FA (*(sp - 1));
    }
	//printf("%d %c \n",i,c);
  }

  stack[0]->fin->terminal=1;
  return stack[0];
}

void help ()
{
  printf ("Enter -o to set output file name.\n");
  printf ("Enter -i to set input file name.\n");
  
}


char reduce_epsilon()
{ 
	char ret=0;
	 
	for(int i=0;i<nctr;++i)
	{ 
		for(edge* it = nodes[i].e; it!=NULL; it=it->next)
		{
			//printf("F");
			if(it->name[0]=='$' && it->to->indeg<=1 && it->to->outdeg<=1 && it->to->e!=NULL)
			{
				printf("!!");
				it->name = it->to->e->name;
				it->to = (it->to->e->to);
				
				ret=1;
			}
		}
	}
	return ret;  
}


int main (int argc, char **argv)
{
  strcpy (out_file, "at.tex");
  strcpy (in_file, "input.txt");


  COUNTER=0;
  char do_opt=1;

  int mode = 1;

  for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
    {
      if (argv[i][1] == 'h')
      {
        help ();
        exit (0);
      }
      if (argv[i][1] == 'o')
      {
        strcpy (out_file, argv[i + 1]);
      }
      if (argv[i][1] == 'i')
      {
        strcpy (in_file, argv[i + 1]);
      }
      if (argv[i][1] == 'r' && argv[i][2] == 'e')
      {
        mode = 1;
      }
    }
  }

  if (mode == 1)
  {
    read_reg_exp ();
    rexp_to_rpn (); 
    printf ( "%s\n[%s]", reg_exp, reg_exp_rpn);
    

    FA* fa = build_NFA_rexp();
    if(do_opt)
    { 
		reduce_epsilon(); 
	}
    out_graph (fa->start);

    printf ("Done.\n");
    exit (0);
  }

  printf ("Unknown command. Type graph-builder -h to get gelp.\n");
  return 0;
}
