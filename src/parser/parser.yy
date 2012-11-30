%{

#include <sstream>
#include "GlobalData.hpp"
#include "common.hpp"
#include "insert.hpp"
#include "theory/term.hpp" // Necessary for inheritance tree
#include "parser/yyltype.hpp"

// Lexer
extern int yylex();

Insert& data();

std::string getInstalledFilePath(std::string filename);

// Errors
void yyerror(const char* s);

# define YYLLOC_DEFAULT(Current, Rhs, N)									 \
         do                                                                  \
           if (N)                                                            \
             {                                                               \
               (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;         \
               (Current).first_column = YYRHSLOC(Rhs, 1).first_column;       \
               (Current).last_line    = YYRHSLOC(Rhs, N).last_line;          \
               (Current).last_column  = YYRHSLOC(Rhs, N).last_column;        \
			   (Current).descr		  = YYRHSLOC(Rhs, 1).descr;				 \
             }                                                               \
           else                                                              \
             {                                                               \
               (Current).first_line   = (Current).last_line   =              \
                 YYRHSLOC(Rhs, 0).last_line;                                 \
               (Current).first_column = (Current).last_column =              \
                 YYRHSLOC(Rhs, 0).last_column;                               \
			   (Current).descr		  = YYRHSLOC(Rhs,0).descr;				 \
             }                                                               \
         while (0)

%}

/** Produce readable error messages **/
%debug
%locations
%error-verbose

/** Data structures **/
%union{
	int						nmr;
	char					chr;
	double					dou;
	std::string*			str;

	InternalArgument*		arg;

	Sort*					sor;
	Predicate*				pre;
	Function*				fun;
	const Compound*			cpo;
	Term*					ter;
	Rule*					rul;
	FixpDef*				fpd;
	Definition*				def;
	Formula*				fom;
	Query*					que;
	Variable*				var;
	SetExpr*				set;
	EnumSetExpr*			est;
	NSPair*					nsp;
	SortTable*				sta;
	PredTable*				pta;
	FuncTable*				fta;
	const DomainElement*	dom;

	std::vector<std::string>*			vstr;
	std::vector<Sort*>*					vsor;
	std::set<Variable*>*				svar;
	std::vector<Variable*>*				vvar;
	std::vector<Term*>*					vter;
	std::vector<Formula*>*				vfom;
	std::vector<Rule*>*					vrul;
	std::pair<int,int>*					vint;
	std::pair<char,char>*				vcha;
	std::vector<const DomainElement*>*	vdom;
	std::vector<ElRange>*				vera;
	std::stringstream*					sstr;

	std::vector<std::vector<std::string> >* vvstr;

}

/** Headers  **/
%token VOCAB_HEADER
%token THEORY_HEADER
%token STRUCT_HEADER
%token ASP_HEADER
%token NAMESPACE_HEADER
%token PROCEDURE_HEADER
%token QUERY_HEADER
%token TERM_HEADER

/** Keywords **/
%token CONSTRUCTOR
%token PROCEDURE
%token PARTIAL
%token EXTENDS
%token EXTERN
%token P_MINAGG P_MAXAGG P_CARD P_PROD P_SOM
%token FALSE
%token USINGVOCABULARY
%token EXTERNVOCABULARY
%token USINGNAMESPACE
%token TYPE
%token TRUE
%token ABS
%token ISA
%token LFD
%token GFD
%token NEWLINE
%token LUAVARARG

/** Other Terminals **/
%token <nmr> INTEGER
%token <dou> FLNUMBER
%token <chr> CHARACTER
%token <chr> CHARCONS
%token <str> IDENTIFIER
%token <str> STRINGCONS
%token <sstr>	LUACHUNK

/** Aliases **/
%token <operator> MAPS			"->"
%token <operator> EQUIV			"<=>"
%token <operator> P_IMPL		"=>"
%token <operator> P_RIMPL		"<="
%token <operator> DEFIMP		"<-"
%token <operator> P_NEQ			"~="
%token <operator> P_EQ			"=="
%token <operator> P_LEQ			"=<"
%token <operator> P_GEQ			">="
%token <operator> RANGE			".."
%token <operator> NSPACE		"::"

/** Precedence declarations for connectives (higher line number = higher precedence)  **/
%right ':'
%right "<=>"
%right "=>"
%right "<="
%right '|'
%right '&'
%right '~' 

/**  Precedence declarations for arithmetic (higher line number = higher precedence) **/
%left '-' '+'
%left '/' '*'
%left '%' 
%left '^'
%left UMINUS

/** Non-terminals with semantic value **/
%type <nmr> integer
%type <dou> floatnr
%type <str> strelement
%type <str>	identifier
%type <var> variable
%type <sor> sort_pointer
%type <sor> sort_decl
%type <sor> theosort_pointer
%type <pre> pred_decl
%type <nsp>	intern_pointer
%type <fun> func_decl
%type <fun> full_func_decl
%type <fun> arit_func_decl
%type <ter> term domterm function arterm aggterm
%type <fom> predicate
%type <fom> head
%type <fom> formula
%type <fom>	eq_chain
%type <rul> rule
%type <sta> elements_es
%type <sta> elements
%type <fpd> fixpdef
%type <fpd> fd_rules
%type <def> definition
%type <est> formulaset
%type <est> termset
%type <est> form_list
%type <est> form_term_list
%type <cpo> compound
%type <arg> function_call
%type <pta>	ptuples
%type <pta>	ptuples_es
%type <fta> ftuples
%type <fta> ftuples_es
%type <pta> f3tuples
%type <pta> f3tuples_es
%type <dom>	pelement
%type <dom>	domain_element
%type <que> query

%type <vint>	intrange
%type <vcha>	charrange
%type <vter>	term_tuple
%type <svar>	variables univquantvars
%type <vvar>	query_vars
%type <vrul>	rules
%type <vstr>	pointer_name
%type <vsor>	sort_pointer_tuple
%type <vsor>	nonempty_spt
%type <vsor>	binary_arit_func_sorts
%type <vsor>	unary_arit_func_sorts
%type <vdom>	compound_args
%type <vdom>	ptuple	
%type <vdom>	ftuple	
%type <vera>	domain_tuple

%type <vvstr> pointer_names

%%

/*********************
	Global structure
*********************/

idp		: /* empty */
				| idp namespace 
				| idp vocabulary 
				| idp theory
				| idp structure
				| idp asp_structure
				| idp instructions
				| idp namedquery
				| idp namedterm
				| idp using
		        ;
	
namespace		: NAMESPACE_HEADER namespace_name '{' idp '}'	{ data().closeNamespace();	}
				;

namespace_name	: identifier									{ data().openNamespace(*$1,@1); }
				;

using			: USINGNAMESPACE pointer_name					{ data().usingspace(*$2,@1); delete($2);	}
				| USINGVOCABULARY pointer_name					{ data().usingvocab(*$2,@1); delete($2);	}
				;

 
/***************************
	Vocabulary declaration
***************************/

/** Structure of vocabulary declaration **/

vocabulary			: VOCAB_HEADER vocab_name '{' vocab_content '}'		{ data().closevocab();	}	
					| VOCAB_HEADER vocab_name '=' function_call			{ data().assignvocab($4,@2);
																		  data().closevocab();	}
					;

vocab_name			: identifier	{ data().openvocab(*$1,@1); }
					;

vocab_pointer		: pointer_name	{ data().setvocab(*$1,@1); delete($1); }
					;

vocab_content		: /* empty */
					| vocab_content NEWLINE
					| vocab_content symbol_declaration NEWLINE
					| vocab_content EXTERN extern_symbol NEWLINE 
					| vocab_content EXTERNVOCABULARY pointer_name	{ data().externvocab(*$3,@3); delete($3);	} NEWLINE
					| vocab_content using NEWLINE
					;

symbol_declaration	: sort_decl
					| pred_decl
					| func_decl
					;

extern_symbol		: extern_sort
					| extern_predicate
					| extern_function
					;

extern_sort			: TYPE sort_pointer	{ data().sort($2);			}
					;

extern_predicate	: pointer_name '[' sort_pointer_tuple ']'	{ data().predicate(data().predpointer(*$1,*$3,@1));
																  delete($1); delete($3); }
					| pointer_name '/' INTEGER					{ data().predicate(data().predpointer(*$1,$3,@1));
																  delete($1); }
					;

extern_function		: pointer_name '[' sort_pointer_tuple ':' sort_pointer ']' 
																	{ data().function(data().funcpointer(*$1,*$3,@1));
																	  delete($1); delete($3); }
					| pointer_name '/' INTEGER ':' INTEGER			{ data().function(data().funcpointer(*$1,$3,@1));
																	  delete($1); }
					;

/** Symbol declarations **/

sort_decl		: TYPE identifier											{ $$ = data().sort(*$2,@2);		}
				| TYPE identifier ISA nonempty_spt							{ $$ = data().sort(*$2,*$4,true,@2);	
																			  delete($4); }
				| TYPE identifier EXTENDS nonempty_spt						{ $$ = data().sort(*$2,*$4,false,@2);	
																			  delete($4); }
				| TYPE identifier ISA nonempty_spt EXTENDS nonempty_spt		{ $$ = data().sort(*$2,*$4,*$6,@2);		
																			  delete($4); delete($6); }
				| TYPE identifier EXTENDS nonempty_spt ISA nonempty_spt		{ $$ = data().sort(*$2,*$6,*$4,@2);		
																			  delete($4); delete($6); }
				;

pred_decl		: identifier '(' sort_pointer_tuple ')'	{ data().predicate(*$1,*$3,@1); delete($3); }
				| identifier							{ data().predicate(*$1,@1);  }
				;

func_decl		: PARTIAL full_func_decl				{ $$ = $2; data().partial($$);	}
				| full_func_decl						{ $$ = $1;								}
				| PARTIAL arit_func_decl				{ $$ = $2; data().partial($$);	}
				| arit_func_decl						{ $$ = $1;								}
				;

full_func_decl	: identifier '(' sort_pointer_tuple ')' ':' sort_pointer	{ $$ = data().function(*$1,*$3,$6,@1);
																			  delete($3); }
				| identifier ':' sort_pointer								{ $$ = data().function(*$1,$3,@1); }	
				; 														

arit_func_decl	: '-' binary_arit_func_sorts				{ $$ = data().aritfunction("-/2",*$2,@1); delete($2);	}
                | '+' binary_arit_func_sorts				{ $$ = data().aritfunction("+/2",*$2,@1); delete($2);	}
                | '*' binary_arit_func_sorts				{ $$ = data().aritfunction("*/2",*$2,@1); delete($2);	}
                | '/' binary_arit_func_sorts				{ $$ = data().aritfunction("//2",*$2,@1); delete($2);	}
                | '%' binary_arit_func_sorts				{ $$ = data().aritfunction("%/2",*$2,@1); delete($2);	}
                | '^' binary_arit_func_sorts				{ $$ = data().aritfunction("^/2",*$2,@1); delete($2);	}
                | '-' unary_arit_func_sorts  %prec UMINUS	{ $$ = data().aritfunction("-/1",*$2,@1); delete($2);	}
                | ABS unary_arit_func_sorts					{ $$ = data().aritfunction("abs/1",*$2,@1); delete($2);	}
				;

binary_arit_func_sorts	: '(' sort_pointer ',' sort_pointer ')' ':' sort_pointer	
							{ $$ = new std::vector<Sort*>(3); (*$$)[0] = $2; (*$$)[1] = $4; (*$$)[2] = $7; }
						;

unary_arit_func_sorts	: '(' sort_pointer ')' ':' sort_pointer	
							{ $$ = new std::vector<Sort*>(2); (*$$)[0] = $2; (*$$)[1] = $5; }
						;

/** Symbol pointers **/

sort_pointer		: pointer_name				{ $$ = data().sortpointer(*$1,@1); delete($1); }
					;

intern_pointer		: pointer_name '[' sort_pointer_tuple ']'	{ $$ = data().internpredpointer(*$1,*$3,@1);
																  delete($1); delete($3); }
					| pointer_name '[' sort_pointer_tuple ':' sort_pointer ']'	
																{ $$ = data().internfuncpointer(*$1,*$3,$5,@1);
																  delete($1); delete($3); }
					| pointer_name								{ $$ = data().internpointer(*$1,@1);
																  delete($1); }
					;

pointer_name		: pointer_name "::" identifier	{ $$ = $1; $$->push_back(*$3); 		}
					| identifier					{ $$ = new std::vector<std::string>(1,*$1);	}
					;

/**************
	Queries
**************/

namedquery	: QUERY_HEADER query_name ':' vocab_pointer '{' query '}'	{ data().closequery($6);	}
			;

query_name	: identifier	{ data().openquery(*$1,@1);	}
			;

query		: '{' query_vars ':' formula '}'		{ $$ = data().query(*$2,$4,@1); delete($2);	}
			;

query_vars	: /* empty */			{ $$ = new std::vector<Variable*>(0);	}
			| query_vars variable	{ $$ = $1; $$->push_back($2);		}
			;

/******************
	Named terms
******************/

namedterm	: TERM_HEADER term_name ':' vocab_pointer '{' term '}'	{ data().closeterm($6);	}
			;

term_name	: identifier	{ data().openterm(*$1,@1);	}
			;

/*************
	Theory	
*************/

theory		: THEORY_HEADER theory_name ':' vocab_pointer '{' def_forms '}'		{ data().closetheory();	}
			| THEORY_HEADER theory_name '=' function_call						{ data().assigntheory($4,@2); 
																				  data().closetheory();	}
			;

theory_name	: identifier	{ data().opentheory(*$1,@1);  }
			;

function_call	: pointer_name '(' pointer_names ')'	{ $$ = data().call(*$1,*$3,@1); delete($1); delete($3);	}
				| pointer_name '(' ')'					{ $$ = data().call(*$1,@1); delete($1);					}
				;

pointer_names	: pointer_names ',' pointer_name	{ $$ = $1; $$->push_back(*$3);
													  delete($3); }
				| pointer_name						{ $$ = new std::vector<std::vector<std::string> >(1,*$1);
													  delete($1); }
				;

def_forms	: /* empty */
			| def_forms definition		{ data().definition($2);	}
			| def_forms formula '.'		{ data().sentence($2);		}
			| def_forms fixpdef			{ data().fixpdef($2);		}
			| def_forms using
			;

/** Definitions **/

definition	: '{' rules '}'		{ $$ = data().definition(*$2); delete($2);	}
			| '{' '}' 			{ $$ = NULL;} //Ignore empty definitions (the inserter can handle NULL)	
			;

rules		: rules rule	{ $$ = $1; $1->push_back($2);	}				
			| rule			{ $$ = new std::vector<Rule*>(1,$1);	}			
			;

rule		: univquantvars head "<-" formula '.'	{ $$ = data().rule(*$1,$2,$4,@1); delete($1);	}
			| univquantvars head "<-"		  '.'	{ $$ = data().rule(*$1,$2,@1); delete($1);		}
			| univquantvars head			  '.'	{ $$ = data().rule(*$1,$2,@1); delete($1);		}
			;
			
univquantvars 
			: univquantvars '!' variables ':' 	{ $$ = $1; $1->insert($3->cbegin(), $3->cend()); delete($3);}
			|  /* empty*/ 						{ $$ = new std::set<Variable*>(); }
			;

head		: predicate				{ $$ = $1;										}
			| function '=' term		{ $$ = data().equalityhead($1,$3,@1);	}
			;
			

/** Fixpoint definitions **/

fixpdef		: LFD '[' fd_rules ']'		{ $$ = $3; data().makeLFD($3,true); 	}
			| GFD '[' fd_rules ']'		{ $$ = $3; data().makeLFD($3,false);	}
			;

fd_rules	: fd_rules rule	'.'			{ $$ = $1; data().addRule($$,$2);					}
			| fd_rules fixpdef			{ $$ = $1; data().addDef($$,$2);					}
			| rule '.'					{ $$ = data().createFD(); data().addRule($$,$1);	}
			| fixpdef					{ $$ = data().createFD(); data().addDef($$,$1);		}
			;	

/** Formulas **/

formula		: '!' variables ':' formula					{ $$ = data().univform(*$2,$4,@1); delete($2);		}
            | '?' variables ':' formula					{ $$ = data().existform(*$2,$4,@1); delete($2);	}
			| '?' INTEGER  variables ':' formula		{ $$ = data().bexform(CompType::EQ,$2,*$3,$5,@1);
														  delete($3);										}
			| '?' '=' INTEGER variables ':' formula		{ $$ = data().bexform(CompType::EQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' '<' INTEGER variables ':' formula		{ $$ = data().bexform(CompType::LT,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' '>' INTEGER variables ':' formula		{ $$ = data().bexform(CompType::GT,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' "=<" INTEGER variables ':' formula	{ $$ = data().bexform(CompType::LEQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' ">=" INTEGER variables ':' formula	{ $$ = data().bexform(CompType::GEQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '~' formula								{ $$ = $2; data().negate($$);						}
            | formula '&' formula						{ $$ = data().conjform($1,$3,@1);					}
            | formula '|' formula						{ $$ = data().disjform($1,$3,@1);					}
            | formula "=>" formula						{ $$ = data().implform($1,$3,@1);					}
            | formula "<=" formula						{ $$ = data().revimplform($1,$3,@1);				}
            | formula "<=>" formula						{ $$ = data().equivform($1,$3,@1);					}
            | '(' formula ')'							{ $$ = $2;											}
			| TRUE										{ $$ = data().trueform(@1);						}
			| FALSE										{ $$ = data().falseform(@1);						}
			| eq_chain									{ $$ = $1;											}
            | predicate									{ $$ = $1;											}
            ;

predicate   : intern_pointer							{ $$ = data().predform($1,@1);					}
			| intern_pointer '(' ')'					{ $$ = data().predform($1,@1);					}
            | intern_pointer '(' term_tuple ')'			{ $$ = data().predform($1,*$3,@1); delete($3); }
            ;

eq_chain	: eq_chain '='  term	{ $$ = data().eqchain(CompType::EQ,$1,$3,@1);	}
			| eq_chain "~=" term	{ $$ = data().eqchain(CompType::NEQ,$1,$3,@1);	}
			| eq_chain '<'  term	{ $$ = data().eqchain(CompType::LT,$1,$3,@1);	}	
			| eq_chain '>'  term    { $$ = data().eqchain(CompType::GT,$1,$3,@1);	}
			| eq_chain "=<" term	{ $$ = data().eqchain(CompType::LEQ,$1,$3,@1);	}		
			| eq_chain ">=" term	{ $$ = data().eqchain(CompType::GEQ,$1,$3,@1);	}		
			| term '='  term		{ $$ = data().eqchain(CompType::EQ,$1,$3,@1);	}
            | term "~=" term		{ $$ = data().eqchain(CompType::NEQ,$1,$3,@1);	}
            | term '<'  term		{ $$ = data().eqchain(CompType::LT,$1,$3,@1);	}
            | term '>'  term		{ $$ = data().eqchain(CompType::GT,$1,$3,@1);	}
            | term "=<" term		{ $$ = data().eqchain(CompType::LEQ,$1,$3,@1);	}
            | term ">=" term		{ $$ = data().eqchain(CompType::GEQ,$1,$3,@1);	}
			;

variables   : variables variable	{ $$ = $1; $$->insert($2);						}		
			| variables ',' variable	{ $$ = $1; $$->insert($3);						}
            | variable				{ $$ = new std::set<Variable*>;	$$->insert($1);	}
			;

variable	: identifier							{ $$ = data().quantifiedvar(*$1,@1);		}
			| identifier '[' theosort_pointer ']'	{ $$ = data().quantifiedvar(*$1,$3,@1);	}
			;

theosort_pointer	:	pointer_name		{ $$ = data().theosortpointer(*$1,@1); delete($1);	}
					;

/** Terms **/                                            

term		: function		{ $$ = $1;	}		
			| arterm		{ $$ = $1;	}
			| domterm		{ $$ = $1;	}
			| aggterm		{ $$ = $1;	}
			;

function	: intern_pointer '(' term_tuple ')'		{ $$ = data().functerm($1,*$3); delete($3);	}
			| intern_pointer '(' ')'				{ $$ = data().term($1);					}
			| intern_pointer						{ $$ = data().term($1);					}
			;

arterm		: term '-' term				{ $$ = data().arterm('-',$1,$3,@1);	}				
			| term '+' term				{ $$ = data().arterm('+',$1,$3,@1);	}
			| term '*' term				{ $$ = data().arterm('*',$1,$3,@1);	}
			| term '/' term				{ $$ = data().arterm('/',$1,$3,@1);	}
			| term '%' term				{ $$ = data().arterm('%',$1,$3,@1);	}
			| term '^' term				{ $$ = data().arterm('^',$1,$3,@1);	}
			| '-' term %prec UMINUS		{ $$ = data().arterm("-",$2,@1);		}
			| ABS '(' term ')'			{ $$ = data().arterm("abs",$3,@1);		}
			| '(' arterm ')'			{ $$ = $2;								}
			;

domterm		: INTEGER									{ $$ = data().domterm($1,@1);		}
			| FLNUMBER									{ $$ = data().domterm($1,@1);		}
			| STRINGCONS								{ $$ = data().domterm($1,@1);		}
			| CHARCONS									{ $$ = data().domterm($1,@1);		}
		/*	| '@' identifier '[' theosort_pointer ']'	{ $$ = data().domterm($2,$4,@1);	}
			| '@' identifier							{ $$ = data().domterm($2,0,@1);	}
		The above lines are commented since writing @ is unsafe"*/
			;

aggterm		: P_CARD formulaset	{ $$ = data().aggregate(AggFunction::CARD,$2,@1);	}
			| P_SOM termset		{ $$ = data().aggregate(AggFunction::SUM,$2,@1);		}
			| P_PROD termset	{ $$ = data().aggregate(AggFunction::PROD,$2,@1);	}
			| P_MINAGG termset	{ $$ = data().aggregate(AggFunction::MIN,$2,@1);		}
			| P_MAXAGG termset	{ $$ = data().aggregate(AggFunction::MAX,$2,@1);		}
			;

formulaset		: '{' variables ':' formula '}'				{ $$ = data().set(*$2,$4,@1); delete($2);	}
				| '[' form_list ']'							{ $$ = $2;											}
				;

termset			: '{' variables ':' formula ':' term '}'	{ $$ = data().set(*$2,$4,$6,@1); delete($2);	}	
				| '[' form_term_list ']'					{ $$ = $2;											}
				;

form_list		: form_list ';' formula						{ $$ = $1; data().addFormula($$,$3);						}
				| formula									{ $$ = data().createEnum(@1); $$=data().addFormula($$,$1);	}		
				;

form_term_list	: form_term_list ';' '(' formula ',' term ')'	{ $$ = $1; data().addFT($$,$4,$6);						}
				| '(' formula ',' term ')'						{ $$ = data().createEnum(@1); $$ = data().addFT($$,$2,$4);	}
				;


/**********************
	Input structure
**********************/

structure		: STRUCT_HEADER struct_name ':' vocab_pointer '{' interpretations '}'	{ data().closestructure();	}
				| STRUCT_HEADER struct_name '=' function_call							{ data().assignstructure($4,@2);
																						  data().closestructure();	}
				;

struct_name		: identifier	{ data().openstructure(*$1,@1); }
				;

interpretations	:  //empty
				| interpretations interpretation
				| interpretations using
				;

interpretation	: empty_inter
				| sort_inter
				| pred_inter
				| func_inter
				| proc_inter
				| three_inter
				;

/** Empty interpretations **/

empty_inter	: intern_pointer '=' '{' '}'				{ data().emptyinter($1); }
			;

/** Interpretations with arity 1 **/

sort_inter	: intern_pointer '=' '{' elements_es '}'		{ data().sortinter($1,$4);  }	
			;

elements_es		: elements ';'						{ $$ = $1;	}
				| elements							{ $$ = $1;	}
				;

elements		: elements ';' charrange			{ $$ = $1; data().addElement($$,$3->first,$3->second); delete($3);	}
				| elements ';' intrange				{ $$ = $1; data().addElement($$,$3->first,$3->second); delete($3);	}
				| elements ';' '(' strelement ')'	{ $$ = $1; data().addElement($$,$4);						}
				| elements ';' '(' integer ')'		{ $$ = $1; data().addElement($$,$4);						}
				| elements ';' '(' compound ')'		{ $$ = $1; data().addElement($$,$4);						}
				| elements ';' '(' floatnr ')'		{ $$ = $1; data().addElement($$,$4);						}
				| elements ';' integer				{ $$ = $1; data().addElement($$,$3);						}
				| elements ';' strelement			{ $$ = $1; data().addElement($$,$3);						}
				| elements ';' floatnr				{ $$ = $1; data().addElement($$,$3);						}
				| elements ';' compound				{ $$ = $1; data().addElement($$,$3);						}
				| charrange							{ $$ = data().createSortTable(); 
													  data().addElement($$,$1->first,$1->second); delete($1);	}
				| intrange							{ $$ = data().createSortTable(); 
													  data().addElement($$,$1->first,$1->second); delete($1);	}
				| '(' strelement ')'				{ $$ = data().createSortTable(); data().addElement($$,$2);	}
				| '(' integer ')'					{ $$ = data().createSortTable(); data().addElement($$,$2);	}
				| '(' floatnr ')'                   { $$ = data().createSortTable(); data().addElement($$,$2);	}
				| '(' compound ')'					{ $$ = data().createSortTable(); data().addElement($$,$2);	}
				| strelement						{ $$ = data().createSortTable(); data().addElement($$,$1);	}
				| integer							{ $$ = data().createSortTable(); data().addElement($$,$1);	}
				| floatnr							{ $$ = data().createSortTable(); data().addElement($$,$1);	}
				| compound							{ $$ = data().createSortTable(); data().addElement($$,$1);	}
				;

strelement		: identifier	{ $$ = $1;									}
				| STRINGCONS	{ $$ = $1;									}
				| CHARCONS		{ $$ = StringPointer(std::string(1,$1));	}
				;

/** Interpretations with arity not 1 **/

pred_inter		: intern_pointer '=' '{' ptuples_es '}'		{ data().predinter($1,$4);		}
				| intern_pointer '=' TRUE					{ data().truepredinter($1);		}
				| intern_pointer '=' FALSE					{ data().falsepredinter($1);	}
				;

ptuples_es		: ptuples ';'					{ $$ = $1;	}	
				| ptuples						{ $$ = $1;	}
				;

ptuples			: ptuples ';' '(' ptuple ')'	{ $$ = $1; data().addTuple($$,*$4,@4); delete($4);	}
				| ptuples ';' ptuple			{ $$ = $1; data().addTuple($$,*$3,@3); delete($3);	}
				| ptuples ';' emptyptuple		{ $$ = $1; data().addTuple($$,@3);					}
				| '(' ptuple ')'				{ $$ = data().createPredTable($2->size()); data().addTuple($$,*$2,@2); delete($2);	}
				| ptuple						{ $$ = data().createPredTable($1->size()); data().addTuple($$,*$1,@1); delete($1);	}
				| emptyptuple					{ $$ = data().createPredTable(0); data().addTuple($$,@1);					}
				;

emptyptuple		: '(' ')'										
				;

ptuple			: ptuple ',' pelement			{ $$ = $1; $$->push_back($3);	}			
				| pelement ',' pelement			{ $$ = new std::vector<const DomainElement*>(); 
												  $$->push_back($1); $$->push_back($3); }
				;

pelement		: integer		{ $$ = data().element($1);	}
				| identifier	{ $$ = data().element($1);	}
				| CHARCONS		{ $$ = data().element($1);	}
				| STRINGCONS	{ $$ = data().element($1);	}
				| floatnr		{ $$ = data().element($1);	}
				| compound		{ $$ = data().element($1);	}
				;

/** Interpretations for functions **/

func_inter	: intern_pointer '=' '{' ftuples_es '}'	{ data().funcinter($1,$4); }	
			| intern_pointer '=' pelement			{ FuncTable* ft = data().createFuncTable(1);
													  data().addTupleVal(ft,$3,@3);
													  data().funcinter($1,ft); }
			| intern_pointer '=' CONSTRUCTOR		{ data().constructor($1);	}
			;

ftuples_es		: ftuples ';'					{ $$ = $1;	}
				| ftuples						{ $$ = $1;	}
				;

ftuples			: ftuples ';' ftuple			{ $$ = $1; data().addTupleVal($$,*$3,@3); delete($3);					    }
				| ftuple						{ $$ = data().createFuncTable($1->size()); data().addTupleVal($$,*$1,@1);	delete($1);	}
				;

ftuple			: ptuple "->" pelement			{ $$ = $1; $$->push_back($3);	}			
				| pelement "->" pelement		{ $$ = new std::vector<const DomainElement*>(1,$1); $$->push_back($3);	}
				| emptyptuple "->" pelement		{ $$ = new std::vector<const DomainElement*>(1,$3);	}		
				| "->" pelement					{ $$ = new std::vector<const DomainElement*>(1,$2);	}
				;

f3tuples_es		: f3tuples ';'					{ $$ = $1;	}
				| f3tuples						{ $$ = $1;	}
				;

f3tuples		: f3tuples ';' ftuple			{ $$ = $1; data().addTuple($$,*$3,@3); delete($3);							}
				| ftuple						{ $$ = data().createPredTable($1->size()); data().addTuple($$,*$1,@1); delete($1);	}
				;

/** Procedural interpretations **/

proc_inter		: intern_pointer '=' PROCEDURE pointer_name	{ data().inter($1,*$4,@1); delete($4);	}
				;

/** Three-valued interpretations **/

three_inter		: threepred_inter
				| threefunc_inter
				| threeempty_inter
				;

threeempty_inter	: intern_pointer '<' identifier '>' '=' '{' '}'			{ data().emptythreeinter($1,*$3); }
					;

threepred_inter : intern_pointer '<' identifier '>' '=' '{' ptuples_es '}'	{ data().threepredinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' '{' elements_es '}'	{ data().threepredinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' TRUE				{ data().truethreepredinter($1,*$3);	}
				| intern_pointer '<' identifier '>' '=' FALSE				{ data().falsethreepredinter($1,*$3);	}
				;

threefunc_inter	: intern_pointer '<' identifier '>' '=' '{' f3tuples_es '}'	{ data().threefuncinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' pelement			{ PredTable* ft = data().createPredTable(1);
																			  std::vector<const DomainElement*> vd(1,$6);
																			  data().addTuple(ft,vd,@6);
																			  data().threefuncinter($1,*$3,ft);		}
				;

/** Ranges **/

intrange	: integer ".." integer			{ $$ = data().range($1,$3,@1); }
			;
charrange	: CHARACTER ".." CHARACTER		{ $$ = data().range($1,$3,@1);	}
			| CHARCONS ".." CHARCONS		{ $$ = data().range($1,$3,@1);	}
			;

/** Compound elements **/

compound	: intern_pointer '(' compound_args ')'	{ $$ = data().compound($1,*$3); delete($3);	}
			| intern_pointer '(' ')'				{ $$ = data().compound($1);					}
			;

compound_args	: compound_args ',' floatnr		{ $$ = $1; $$->push_back(data().element($3));	}
				| compound_args ',' integer		{ $$ = $1; $$->push_back(data().element($3));	}
				| compound_args ',' strelement	{ $$ = $1; $$->push_back(data().element($3));	}
				| compound_args ',' compound	{ $$ = $1; $$->push_back(data().element($3));	}
				| floatnr						{ $$ = new std::vector<const DomainElement*>(1,data().element($1));	}
				| integer						{ $$ = new std::vector<const DomainElement*>(1,data().element($1));	}
				| strelement					{ $$ = new std::vector<const DomainElement*>(1,data().element($1));	}
				| compound						{ $$ = new std::vector<const DomainElement*>(1,data().element($1));	}
				;
	         
/** Terminals **/

integer			: INTEGER		{ $$ = $1;		}
				| '-' INTEGER	{ $$ = -$2;		}
				;

floatnr			: FLNUMBER			{ $$ = $1;		}
				| '-' FLNUMBER		{ $$ = -($2);	}
				;

identifier		: IDENTIFIER	{ $$ = $1;	}
				| CHARACTER		{ $$ = StringPointer(std::string(1,$1)); } 
				;

/********************
	ASP structure
********************/

asp_structure	: ASP_HEADER struct_name ':' vocab_pointer '{' atoms '}'	{ data().closestructure();	}
				;

atoms	: /* empty */
		| atoms atom '.'
		| atoms using
		;

atom	: predatom
		| funcatom
		;

predatom	: intern_pointer '(' domain_tuple ')'		{ data().predatom($1,*$3,true);	delete($3);		}
			| intern_pointer '(' ')'					{ data().predatom($1,true);						}
			| intern_pointer							{ data().predatom($1,true);						}
			| '-' intern_pointer '(' domain_tuple ')'	{ data().predatom($2,*$4,false); delete($4);	}
			| '-' intern_pointer '(' ')'				{ data().predatom($2,false);					}
			| '-' intern_pointer						{ data().predatom($2,false);					}
			; 

funcatom	: intern_pointer '(' domain_tuple ')' '=' domain_element		{ data().funcatom($1,*$3,$6,true); delete($3);	}
			| intern_pointer '(' ')' '=' domain_element				        { data().funcatom($1,$5,true);		}
			| intern_pointer '=' domain_element						        { data().funcatom($1,$3,true);		}
			| '-' intern_pointer '(' domain_tuple ')' '=' domain_element	{ data().funcatom($2,*$4,$7,false);	delete($4);	}
			| '-' intern_pointer '(' ')' '=' domain_element				    { data().funcatom($2,$6,false);		}
			| '-' intern_pointer '=' domain_element						    { data().funcatom($2,$4,false);		}
			;

domain_tuple	: domain_tuple ',' domain_element	{ $$ = data().domaintuple($1,$3);				}
				| domain_tuple ',' intrange			{ $$ = data().domaintuple($1,$3); delete($3);	}
				| domain_tuple ',' charrange		{ $$ = data().domaintuple($1,$3); delete($3);	}
				| domain_element					{ $$ = data().domaintuple($1);					}
				| intrange							{ $$ = data().domaintuple($1); delete($1);		}
				| charrange							{ $$ = data().domaintuple($1); delete($1);		}
				;

domain_element	: strelement	{ $$ = data().element($1); }
				| integer		{ $$ = data().element($1); }
				| floatnr		{ $$ = data().element($1); }
				| compound		{ $$ = data().element($1); }
				;

term_tuple		: term_tuple ',' term						{ $$ = $1; $$->push_back($3);		}	
				| term										{ $$ = new std::vector<Term*>(1,$1);		}	
				;

sort_pointer_tuple	: /* empty */							{ $$ = new std::vector<Sort*>(0);		}
					| nonempty_spt							{ $$ = $1;							}
					;
					
nonempty_spt		: nonempty_spt ',' sort_pointer	{ $$ = $1; $$->push_back($3);		}
					| sort_pointer					{ $$ = new std::vector<Sort*>(1,$1);		}
					;


/*******************
	Instructions
*******************/

instructions		: PROCEDURE_HEADER proc_name proc_sig '{' LUACHUNK 		{ data().closeprocedure($5); delete($5);	}
					;

proc_name			: identifier	{ data().openprocedure(*$1,@1);	}
					;

proc_sig			: '(' ')'		
					| '(' args ')'
					;

args				: args ',' identifier	{ data().procarg(*$3);		}
					| args ',' LUAVARARG	{ data().procarg("...");		}
					| identifier			{ data().procarg(*$1);		}
					| LUAVARARG				{ data().procarg("...");		}
					;

%%

#include <iostream>
#include <string>
#include "errorhandling/error.hpp"

void yyrestart(FILE*);

extern FILE* yyin;
extern bool alreadyParsed(const std::string& filename);

void yyerror(const char* s) {
	ParseInfo pi(yylloc.first_line,yylloc.first_column,data().currfile());
	Error::error(std::string(s), pi);
}

std::string getInstalledFilePath(std::string filename){
	return getInstallDirectoryPath() + "share/std/" + filename + ".idp"; 
}

std::string state = "";
void parse(const std::string& file) {
	state = "";
//	Assert(parser.include_buffer_stack.empty());
	if(alreadyParsed(file)) {
		return;
	}
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	delete(yylloc.descr);
	yylloc.descr = NULL;

	std::string temp(state+file);
//	std::cerr <<"Opening PARSE" <<temp <<"\n";
	auto origstate = state;
	auto filep = GlobalData::instance()->openFile(temp.c_str(),"r");
	if(filep){
		auto index = temp.find_last_of('/');
		if(index==std::string::npos){
			state = "";
		}else{
			state = temp.substr(0, index+1);
//			std::cerr <<"State set to " <<state <<"\n";
		}
	}
	
	yyrestart(filep);
	if(not yyin){
		yyrestart(GlobalData::instance()->openFile(getInstalledFilePath(file).c_str(),"r"));
	}
	if (yyin) {
		data().currfile(file);
		yyparse();
		GlobalData::instance()->closeFile(yyin);
	}
	else{
		Error::unexistingfile(temp);
	}
	state = origstate;
}

void parsefile(const std::string& file) {
	parse(file);
}

void parsefiles(const std::vector<std::string>& files) {
	for(auto file : files){
		parse(file);
	}
}

void parsestdin() {
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	delete(yylloc.descr);
	yylloc.descr = NULL;
	yyrestart(stdin);
	data().currfile(NULL);
	yyparse();
}

Insert& data(){
	return GlobalData::instance()->getInserter();
}
