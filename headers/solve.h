#pragma warning( disable : 4267)	// STOP MSVS2005 WARNINGS

/*Edit Functions*/

#include "display.h"

void SetUndo()
{
	bUndo=true;
	undo_message+=message;
	undo_line_size=iLineChars;
}

void Undo()
{
	bUndo=false;
	redo_message=message;
	message=undo_message;
	redo_line_size=iLineChars;
	SetLineChars(undo_line_size);
	SetPatterns();
	SetContactTabInfo();
	SetKeyEdit();
}

void Redo()
{
	bUndo=true;
	undo_message=message;
	message=redo_message;
	SetLineChars(redo_line_size);
	SetPatterns();
	SetContactTabInfo();
	SetKeyEdit();
}

//change letter mapped to symbol
void ChangePlain()
{
#if 0
	SYMBOL symbol;
	DIGRAPH digraph;

	if(iCurSymbol<0) return;
	
	GetDlgItemText(hMainWnd,IDC_MAP_VALUE,szText,10); //get new letter

	//get and update symbol
	if(DIGRAPH_MODE) 
	{
		message.digraph_map.GetDigraph(iCurSymbol,&digraph);
		digraph.plain1=szText[0];
		digraph.plain2=szText[1];
		message.digraph_map.AddDigraph(digraph,0);
		UpdateDigraph(iCurSymbol);
	}

	else 
	{
		message.cur_map.GetSymbol(iCurSymbol,&symbol);
		symbol.plain=szText[0];
		message.cur_map.AddSymbol(symbol,0);
		UpdateSymbol(iCurSymbol);
	}

	//update info
	message.Decode(); 
	SetPlain(); SetText();
	SetTable(); SetFreq();
	SetWordList();
#endif
}

/*Solver Functions*/


//reset window state when solve stops
void StopSolve()
{
	siSolveInfo.running=false;
	message.SetKey(siSolveInfo.best_key);
}

//timer thread proc


char FirstAvailable(char *exclude)
{
	for(int letter=0; letter<26; letter++)
		if(!strchr(exclude,letter+'A'))
			return letter+'A';
	
	return 'Z'+1;
}

void BatchBest()
{
	char *szPlainText;
	
	Message best_msg;
	
	szPlainText=new char[message.GetLength()+((iLines+1)*3)+1];
	BreakText(szPlainText,message.GetPlain());
	
	best_msg=message;
	best_msg.cur_map.FromKey(lprgcBatchBestKey);
	BreakText(szPlainText,best_msg.GetPlain());
	SetClipboardText(szPlainText);
	
	delete[] szPlainText;
}

void StopNotify()
{
	//save best key
	if(siSolveInfo.best_score>iBatchBestScore)
	{
		iBatchBestScore=siSolveInfo.best_score;
		strcpy(lprgcBatchBestKey,siSolveInfo.best_key);
	}
	
	/*if(iBruteSymbols>0) BruteNext(iBruteSymbols-1); //in brute
	if(iBruteSymbols<0) //brute finished, display best
	{
		MessageBox(hMainWnd,"Brute Force Done","Alert",MB_OK);
		message.cur_map.FromKey(lprgcBatchBestKey);
		SetDlgInfo();
		SetDlgItemInt(hMainWnd,IDC_SCORE,iBatchBestScore,false);
		iBruteSymbols=0;
	}*/
}

//only include letters, and convert to upper case
int FormatKey(char *key)
{
	int length=strlen(key), temp_index=0;
	char cur_char, *temp=new char[length+1];

	for(int index=0; index<length; index++)
	{
		cur_char=key[index];

		if(IS_LOWER_LTR(cur_char)) cur_char-=32;
		if(!IS_UPPER_LTR(cur_char)) continue;
		temp[temp_index++]=cur_char;
	}

	temp[temp_index]='\0'; 	strcpy(key,temp); delete temp;

	return temp_index;
}

//only include letters, and convert to upper case
int StripWS(char *string)
{
	int length=strlen(string), temp_index=0;
	char *temp=new char[length+1];

	for(int index=0; index<length; index++)
		if(string[index]!=' ' && string[index]!='\r' && string[index]!='\n') temp[temp_index++]=string[index];

	temp[temp_index]='\0'; strcpy(string,temp); delete temp;

	return temp_index;
}

//solve thread proc
int FindSolution() 
{
	int num_symbols;
	char key[4096];
	char *exclude=NULL, *key_text;
	SYMBOL symbol;
	FILE *key_file;
	int key_text_size;
	
	if(!bMsgLoaded) return 0;
	


	if(iSolveType==SOLVE_HOMO)
	{
		num_symbols=message.cur_map.GetNumSymbols();
		
		//if best key is blank, set it to empty symbols + extra letters
		if(!strlen(siSolveInfo.best_key)) 
			message.cur_map.ToKey(siSolveInfo.best_key,szExtraLtr);
		
		//key=program map + additional chars of best key
		message.cur_map.ToKey(key,siSolveInfo.best_key+num_symbols);
		
		//setup exclude list
		exclude=new char[27*num_symbols];

		for(int cur_symbol=0; cur_symbol<num_symbols; cur_symbol++)
		{
			message.cur_map.GetSymbol(cur_symbol,&symbol);
			strcpy(exclude+(27*cur_symbol),symbol.exclude);
		}
		
		siSolveInfo.locked=(char*)message.cur_map.GetLocked();
		siSolveInfo.exclude=exclude;
		
		hillclimb(message,message.GetCipher(),message.GetLength(),key,false);
	}
	else 
	{
		message.InitKeys();
		message.GetKey(key,szExtraLtr);
 		hillclimb2(message,iSolveType,key,iLineChars);
	}

	StopSolve(); //reset window state
	
	if(exclude) delete[] exclude;
	
	StopNotify();
	return 0;
}

//reset window state when solve starts
void StartSolve()
{
	siSolveInfo.running=true;
	FindSolution();
}

void Reset() //init solve info
{
	siSolveInfo.cur_tol=0;
	siSolveInfo.cur_tabu=0;
	siSolveInfo.last_time=0;
	siSolveInfo.best_score=0;
	SetDlgInfo();
	tabu_list.clear();
}


int LoadDictionary(char *filename, int show_error)
{
	FILE *dictionary_file;
	char word[64];
	std::string word_str;

	dictionary_file=fopen(filename,"r");

	if(!dictionary_file)
	{	
		printf("Failed to open %s\n", filename);
	}

	for(int i=0; !feof(dictionary_file); i++) 
	{
		fscanf(dictionary_file,"%s",word);
		for (int i = 0; i < strlen(word); i++)
		{
		  if (word[i] >= 97 && word[i] <= 122) word[i] -= 32;
		}
		word_str=word;
		dictionary[word_str]=i;
	}

	fclose(dictionary_file);
	return 1;
}

//set language and load data files
void SetLanguage()
{
	char szLang[8], szGraphBase[32];
	double unigraphs[26];

	switch(iLang)
	{
		case 0: strcpy(szLanguage,"English"); strcpy(szLang,LANG_ENG); siSolveInfo.lang_ioc=(float)IOC_ENG; break;
		case 1: strcpy(szLanguage,"Spanish"); strcpy(szLang,LANG_SPA); siSolveInfo.lang_ioc=(float)IOC_SPA; break;
		case 2: strcpy(szLanguage,"German"); strcpy(szLang,LANG_GER); siSolveInfo.lang_ioc=(float)IOC_GER; break;
		case 3: strcpy(szLanguage,"Italian"); strcpy(szLang,LANG_ITA); siSolveInfo.lang_ioc=(float)IOC_ITA; break;
		case 4: strcpy(szLanguage,"French"); strcpy(szLang,LANG_FRE); siSolveInfo.lang_ioc=(float)IOC_FRE; break;
	}

	siSolveInfo.lang_dioc=(float)DIOC;
	siSolveInfo.lang_chi=(float)CHI;
	siSolveInfo.lang_ent=(float)ENT;
	
	for(int n=1; n<=5; n++)
	{
		if(n==1)  strcpy(szGraphBase,"unigraphs.txt");
		if(n==2)  strcpy(szGraphBase,"bigraphs.txt");
		if(n==3)  strcpy(szGraphBase,"trigraphs.txt");
		if(n==4)  strcpy(szGraphBase,"tetragraphs.txt");
		if(n==5)  strcpy(szGraphBase,"pentagraphs.txt");
		
		sprintf(szGraphName,"%s/%s/%s",LANG_DIR,szLang,szGraphBase);

		if(!ReadNGraphs(szGraphName,n))
		{
			printf("Could not open %s",szGraphName);
			//SendMessage(hMainWnd,WM_CLOSE,0,0);
		}
	}

	dictionary.clear();
	sprintf(szGraphName,"%s/%s/%s",LANG_DIR,szLang,"dictionary.txt");
	LoadDictionary(szGraphName,true);
	sprintf(szGraphName,"%s/%s/%s",LANG_DIR,szLang,"userdict.txt");
	LoadDictionary(szGraphName,false);
	siSolveInfo.dict_words=dictionary.size();
	
	GetUnigraphs(unigraphs);
	message.cur_map.SetUnigraphs(unigraphs);
	message.SetExpFreq();
}

int ToggleLock()
{
	return -1;
}

void LockWord(int lock)
{
	
}
