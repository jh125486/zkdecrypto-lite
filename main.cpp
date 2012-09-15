#include <stdio.h>
#include <ctime>
#include <math.h>
#include <cstdlib>
#include "headers/zodiac.h"
#include "headers/solve.h"
#include "headers/files.h"

void DispAll();
void DispInfo();
void Solve();
void InitKey();

int initCount[26];

unsigned long TimeFunc() {
    return 42;
}

int main (int argc, char** argv)
{
    char filename[1024];

    if (argc != 2 && argc != 4)
    {
        printf("Usage:\n");
        printf("%s filename [(-t | -s | -i) n]\n", argv[0]);
        printf("-t n limits runtime to n seconds\n");
        printf("-s n limits solution score to n\n");
        printf("-i n limits solution to n iterations\n");
        printf("If no limit options are given, solver will run for 2 minutes\n");
    }
    LoadMessage(argv[1], 0);

    srand(time(0));

    /*setup directories*/
    char* cipherDir = "/cipher";
    char* keyDir = "/key";

    StopSolve(); //clean GUI stuff out of that function
    //SetSort(0); //will not need

    memset(&siSolveInfo,0,sizeof(SOLVEINFO));
    siSolveInfo.ioc_weight=siSolveInfo.ent_weight=siSolveInfo.chi_weight=5;
    siSolveInfo.dioc_weight=0;
    siSolveInfo.max_tabu=300;
    siSolveInfo.swaps=5;
    siSolveInfo.max_tol=40;
    siSolveInfo.disp_all=DispAll;
    siSolveInfo.disp_info=DispInfo;
    siSolveInfo.time_func=TimeFunc;
    
    sprintf(siSolveInfo.log_name,"/%s","log.txt");
    siSolveInfo.dictionary=&dictionary;
    siSolveInfo.optima_tabu=&tabu_list;
    SetInfo(&siSolveInfo);
    Reset();

    if (argc == 4)
    {
        if (!strcmp(argv[2], "-t"))
        {
            siSolveInfo.time_limit = atoi(argv[3]);
            siSolveInfo.iteration_limit = -1;
            siSolveInfo.score_limit = -1;
        }
        else if (!strcmp(argv[2], "-s"))
        {
            siSolveInfo.score_limit = atoi(argv[3]);
            siSolveInfo.iteration_limit = -1;
            siSolveInfo.time_limit = -1;
        }
        else if (!strcmp(argv[2], "-i"))
        {
            siSolveInfo.iteration_limit = atoi(argv[3]);
            siSolveInfo.time_limit = -1;
            siSolveInfo.score_limit = -1;
        }
    }
    else if (argc == 2)
    {
        siSolveInfo.time_limit = 120;
        siSolveInfo.iteration_limit = -1;
        siSolveInfo.score_limit = -1;
    }

    //language
    iLang=0;
    LoadCribs();
    LoadINI();
    SetLanguage();

    /*if(DIGRAPH_MODE) SendDlgItemMessage(hMainWnd,IDC_MAP_VALUE,EM_LIMITTEXT,2,0);
    else SendDlgItemMessage(hMainWnd,IDC_MAP_VALUE,EM_LIMITTEXT,1,0);*/

    //sovle parameters
    message.SetBlockSize(1);

    SetSolveTypeFeatures();

    //HERE - instead of message loop, immediately start solving
    InitKey();

    Solve();
    SaveINI();
}

void Solve()
{
    if(!bMsgLoaded) return;
    StartSolve();
}

void DispAll()
{
    SetDlgInfo();
    
}

void DispInfo()
{
    
}

void LetterDist(int target)
{
    int letter, set_symbols=0, max_letter;
    double n[26];

    //calculate real number of occurances for each letter
    for(letter=0; letter<26; letter++)
    {
        n[letter]=(message.cur_map.GetUnigraph(letter)/100)*target;
        set_symbols+=int(n[letter]);
    }

    while(set_symbols<target)
    {
        //find the letter with the highest decimal
        max_letter=0;

        for(letter=0; letter<26; letter++)
            if(DECIMAL(n[letter])>DECIMAL(n[max_letter]))
                max_letter=letter;

        //set that letter to the next whole number
        n[max_letter]=int(n[max_letter])+1;
        set_symbols++;
    }

    for(letter=0; letter<26; letter++)
        initCount[letter] = int(n[letter]);
}

void InitKey()
{
    int cur_sel, num_symbols;
    SYMBOL symbol;

    

    if(iCurTab!=0) return;

    num_symbols=message.cur_map.GetNumSymbols();

    //add symbols
    for(int cur_symbol=0; cur_symbol<num_symbols; cur_symbol++)
    {
        message.cur_map.GetSymbol(cur_symbol,&symbol);
        if(!symbol.plain) symbol.plain=BLANK;

        //symbol is locked
        //if(message.cur_map.GetLock(cur_symbol))
        //sprintf(szText, "%c [%c] %5i",symbol.cipher,symbol.plain,symbol.freq);

        //not locked
        //else sprintf(szText,"%c  %c  %5i",symbol.cipher,symbol.plain,symbol.freq);

    }

    //reset selected symbol


    int letter,  target, total;
    //int num_symbols;

    num_symbols=message.cur_map.GetNumSymbols();
    target=num_symbols;

    LetterDist(target);

    num_symbols=message.cur_map.GetNumSymbols();
    total=0;

    int lprgiInitKey[26];

    for(letter=0; letter<26; letter++)
        total+=lprgiInitKey[letter]= initCount[letter];

    /*if(total>num_symbols) //reduce last edited letter if too many
    		{
    			lprgiInitKey[wParam]-=(total-num_symbols);
    			SetDlgItemInt(hWnd,lprgiInitID[wParam],lprgiInitKey[wParam],false);
    			total=target;
    		}*/

    siSolveInfo.best_key[0]='\0';

    message.cur_map.Init(lprgiInitKey);
    message.Decode();

}
