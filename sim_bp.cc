#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <inttypes.h>
#include "sim_bp.h"

// prediction statistics

class prediction_stats
{

    public:
        size_t m_predictions_bimodal = 0;
        size_t m_mispredictions_bimodal = 0;
        size_t m_predictions_gshare = 0;
        size_t m_mispredictions_gshare = 0;
        size_t m_predictions_hybrid = 0;
        size_t m_mispredictions_hybrid = 0;

};


// bimodal branch predictor 

class bimodal_branch_predictor
{ 

private: 

    size_t *branch_table;


public: 

    char bimodal_prediction;                                                           // stores the bimodal prediction
    prediction_stats m_stats;


    // constructor to initialize the branch history table

    bimodal_branch_predictor(size_t m)
    {

        size_t index_width;

        index_width = pow(2, m);

        branch_table = new size_t[index_width];                                  // dynamically allocate a branch table to update the predictions

        for(size_t i=0; i < index_width; i++)
        {

            branch_table[i] = 2;                                                // initialize all the bimodal counters to 2 ("weakly taken")

        }

    }

    virtual ~bimodal_branch_predictor()                                         // deconstructor 

    {
        delete[] branch_table;
    } 

    uint32_t index_bimodal(uint32_t addr, size_t m)                            // calculates the index based on 'M2' lower pc bits
    {

        return ((addr >> 2) & ((1 << m) - 1));

    }

    virtual void predict(uint32_t addr, size_t m)                               // bimodal prediction algorithm
    {

        m_stats.m_predictions_bimodal++ ;                              // increment the number of predictions (i.e., number of dynamic branches in the trace)

        uint32_t index = index_bimodal(addr, m);                      // returns the index for mapping the branch history table

        switch(branch_table[index])                                  // make a prediction according to the given state
        {
            case 3: 

                bimodal_prediction = 't';

            break;

            case 2:

                bimodal_prediction = 't';

            break;

            case 1:

                bimodal_prediction = 'n';
                
            break;

            case 0:

                bimodal_prediction = 'n';

            break;

        }
        

    }

    char get_bimodal_prediction ()
    {

        return bimodal_prediction;

    }

    virtual void update_branch_table(uint32_t addr, size_t m, char outcome)     // update the branch history table based on the actual outcome
    {

        uint32_t index = index_bimodal(addr, m);                               // returns the index for mapping the branch history table

            switch(branch_table[index])                                           
            {
                case 3: 

                    if(outcome == 'n')
                    {   
                    branch_table[index] = branch_table[index] - 1;             // go to weakly taken state 
                    m_stats.m_mispredictions_bimodal++;
                    }

                    break;

                case 2:

                    if(outcome == 'n')
                    {
                    branch_table[index] = branch_table[index] - 1;             // go to weakly not taken state 
                    m_stats.m_mispredictions_bimodal++;
                    }

                    else branch_table[index] = branch_table[index] + 1;       // go to strongly taken state

                    break;

                case 1:

                    if(outcome == 'n')
                    {
                    branch_table[index] = branch_table[index] - 1;             // go to strongly not taken state 

                    }

                
                    else
                    {
                    branch_table[index] = branch_table[index] + 1;            // go to weakly taken state 

                    m_stats.m_mispredictions_bimodal++;
                    }    

                    break;

                case 0:

                    if(outcome == 't')
                    {
                    branch_table[index] = branch_table[index] + 1;           // go to weakly not taken state 
                    m_stats.m_mispredictions_bimodal++;
                    }
                    break;

        }


    }

    void print_bimodal_contents(size_t m)                                      // print the bimodal prediction contents
    {
        printf("FINAL BIMODAL CONTENTS\n");

        for(size_t i=0; i < pow(2,m); i++)
        {

            printf(" %zu      %zu\n", i, branch_table[i]);

        }
    }


};


// gshare branch predictor 

class gshare_branch_predictor
{ 

private: 

    size_t *branch_table;

public: 

    char gshare_prediction;                                             // stores the prediction for hybrid branch predictor
    prediction_stats m_stats;
    size_t global_history_register = 0;                                // initialize the gloabl history register to 0


    // constructor to initialize the branch history table

    gshare_branch_predictor(size_t m)
    {

        size_t index_width;

        index_width = pow(2, m);

        branch_table = new size_t[index_width];                            // dynamically allocate a branch table to update the predictions
        
        for(size_t i=0; i < index_width; i++)
        {

            branch_table[i] = 2;                                         // initialize all the gshare counters to 2 ("weakly taken")

        }

    }

    virtual ~gshare_branch_predictor()                                  // deconstructor 

    {
        delete[] branch_table;
    } 

    uint32_t index_gshare(uint32_t addr, size_t m, size_t n)           // calculates the index based on 'M1' lower pc bits and 'N' global history bits
    {

        uint32_t M = ((addr >> 2) & ((1 << m)-1));
        uint32_t N = (M >> (m-n));

        size_t xor_result = N ^ global_history_register;

        return ((xor_result << (m-n)) | (M & ((1 << (m-n)) - 1)));

    }
    

    virtual void predict(uint32_t addr, size_t m, size_t n)        // gshare prediction algorithm
    {

        m_stats.m_predictions_gshare++ ;                          // increment the number of predictions (i.e., number of dynamic branches in the trace)

        uint32_t index = index_gshare(addr, m, n);               // returns the index for mapping the branch history table
        
        switch(branch_table[index])                             // make a prediction according to the given state
        {
            case 3:

                gshare_prediction = 't';

            break;

            case 2:

                gshare_prediction = 't';

            break;

            case 1:

                gshare_prediction = 'n'; 

            break;

            case 0:

                gshare_prediction = 'n';

            break;

        }

    }

    char get_gshare_prediction ()
    {

        return gshare_prediction;

    }

    virtual void update_branch_table(uint32_t addr, size_t m, size_t n, char outcome)    // update the branch history table based on the actual outcome
    {

        uint32_t index = index_gshare(addr, m, n);                       // returns the index for mapping the branch history table

            switch(branch_table[index])                                           
            {
                case 3: 

                    if(outcome == 'n')
                    {   
                    branch_table[index] = branch_table[index] - 1;     // go to weakly taken state 
                    m_stats.m_mispredictions_gshare++;
                    }

                    break;

                case 2:

                    if(outcome == 'n')
                    {
                    branch_table[index] = branch_table[index] - 1;       // go to weakly not taken state 
                    m_stats.m_mispredictions_gshare++;
                    }

                    else branch_table[index] = branch_table[index] + 1;    // go to strongly taken state

                    break;

                case 1:

                    if(outcome == 'n')
                    {
                    branch_table[index] = branch_table[index] - 1;        // go to strongly not taken state 

                    }

                
                    else
                    {
                    branch_table[index] = branch_table[index] + 1;       // go to weakly taken state 

                    m_stats.m_mispredictions_gshare++;
                    }    

                    break;

                case 0:

                    if(outcome == 't')
                    {
                    branch_table[index] = branch_table[index] + 1;      // go to weakly not taken state 
                    m_stats.m_mispredictions_gshare++;
                    }
                    break;

        }


    }

    virtual void update_global_history(size_t n, char outcome)
    {

        size_t actual_outcome;

        if(outcome == 't')
        {
            actual_outcome = 1;

        }

        else actual_outcome = 0;

        global_history_register = (global_history_register >> 1) | (actual_outcome << (n-1));    // update the global history register 

    }

    void print_gshare_contents(size_t m)       // print the gshare prediction contents
    {
        printf("FINAL GSHARE CONTENTS\n");

        for(size_t i=0; i < pow(2,m); i++)
        {

            printf(" %zu      %zu\n", i, branch_table[i]);

        }
    }


};

// hybrid branch predictor

class hybrid_branch_predictor
{ 

private: 

    size_t *chooser_table;
  

public: 

    char hybrid_prediction;                                             // stores the prediction for hybrid branch predictor
    size_t sel_gshare;
    prediction_stats m_stats;


    // constructor to initialize the branch history table

    hybrid_branch_predictor(size_t k, size_t m1, size_t n, size_t m2)
    {

        size_t index_width;

        index_width = pow(2, k);

        chooser_table = new size_t[index_width];                     // dynamically allocate a branch table to update the predictions

        for(size_t i=0; i < index_width; i++)
        {

            chooser_table[i] = 1;                                   // initialize all the hybrid counters to 1

        }

    }

    virtual ~hybrid_branch_predictor()                                         // deconstructor 

    {
        delete[] chooser_table;
    } 

    uint32_t index_hybrid(uint32_t addr, size_t k)                           // calculates the index based on 'k' lower pc bits
    {

        return ((addr >> 2) & ((1 << k) - 1));

    }

    virtual size_t select_predictor(uint32_t addr, size_t k, size_t m1, size_t n, size_t m2, char outcome)    // hybrid prediction algorithm
    {

        m_stats.m_predictions_hybrid++ ;                // increment the number of predictions (i.e., number of dynamic branches in the trace)

        uint32_t index = index_hybrid(addr, k);        // returns the index for mapping the branch history table

        switch(chooser_table[index])                  // update the branch history table based on the actual outcome
        {
            case 3:                                  

                sel_gshare = 1;                      // select gshare algorithm 
                
            break;

            case 2:

                sel_gshare = 1;                      // select gshare algorithm 

            break;

            case 1:
                
                sel_gshare = 0;                      // select bimodal algorithm 
                
            break;

            case 0:

                sel_gshare = 0;                      // select bimodal algorithm 

            break;
            

        }

        return sel_gshare;
        

    }

    virtual void predict(char outcome, char prediction_hybrid)
    {

        if(outcome != prediction_hybrid)
        {

            m_stats.m_mispredictions_hybrid++;                      // increment the mispredictions for hybrid predictor

        }


    }

    virtual void update_chooser_table(uint32_t addr,size_t k, char prediction_gshare, char prediction_bimodal, char outcome)    // update the chooser table 
    {
         
        uint32_t index = index_hybrid(addr, k);

        bool gshare_correct, bimodal_correct;

        gshare_correct = (prediction_gshare == outcome);                            // gshare prediction is correct
        bimodal_correct = (prediction_bimodal == outcome);                          // bimodal prediction is correct

        if((gshare_correct) & (!bimodal_correct) & (chooser_table[index] < 3))
        {

            chooser_table[index] = chooser_table[index] + 1;

        }

        else if((!gshare_correct) & (bimodal_correct) & (chooser_table[index] > 0))

        {

            chooser_table[index] = chooser_table[index] - 1;

        }

        else
        {

            chooser_table[index] = chooser_table[index];
        }


    }


    void print_hybrid_contents(size_t k)       // print the hybrid prediction contents
    {
        printf("FINAL CHOOSER CONTENTS\n");

        for(size_t i=0; i < pow(2,k); i++)
        {

            printf(" %zu      %zu\n", i, chooser_table[i]);

        }
    }

};




/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim bimodal 6 gcc_trace.txt
    argc = 4
    argv[0] = "sim"
    argv[1] = "bimodal"
    argv[2] = "6"
    ... and so on
*/
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file
    
    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.bp_name  = argv[1];
    
    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if(strcmp(params.bp_name, "bimodal") == 0)              // Bimodal
    {
        if(argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M2       = strtoul(argv[2], NULL, 10);
        trace_file      = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
    }
    else if(strcmp(params.bp_name, "gshare") == 0)          // Gshare
    {
        if(argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M1       = strtoul(argv[2], NULL, 10);
        params.N        = strtoul(argv[3], NULL, 10);
        trace_file      = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);

    }
    else if(strcmp(params.bp_name, "hybrid") == 0)          // Hybrid
    {
        if(argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.K        = strtoul(argv[2], NULL, 10);
        params.M1       = strtoul(argv[3], NULL, 10);
        params.N        = strtoul(argv[4], NULL, 10);
        params.M2       = strtoul(argv[5], NULL, 10);
        trace_file      = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);

    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }

    // dynamic memory allocations

    bimodal_branch_predictor *bimodal = nullptr;                    // bimodal pointer pointing to the "bimodal_branch_predictor" class
    gshare_branch_predictor *gshare = nullptr;                      // ghare pointer pointing to the "gshare_branch_predcitor" class
    hybrid_branch_predictor *hybrid = nullptr;                      // hybrid pointer pointing to the "hybrid_branch_predcitor" class

    if(strcmp(params.bp_name, "bimodal") == 0)                      // bimodal
    {
        bimodal = new bimodal_branch_predictor(params.M2);          // call the constructor to initialize the branch history table
    }
    
    if(strcmp(params.bp_name, "gshare") == 0)                       // gshare
    {
        gshare = new gshare_branch_predictor(params.M1);            // call the constructor to initialize the branch history table
    }

    if(strcmp(params.bp_name, "hybrid") == 0)                       // hybrid
    {
        bimodal = new bimodal_branch_predictor(params.M2);                                // call the constructor to initialize the branch history table
        gshare = new gshare_branch_predictor(params.M1);                                 // call the constructor to initialize the branch history table
        hybrid = new hybrid_branch_predictor(params.K, params.M1, params.N, params.M2);  // call the constructor to initialize the branch history table
    }



    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    char str[2];
    while(fscanf(FP, "%lx %s", &addr, str) != EOF)
    {

        outcome = str[0];

        // if (outcome == 't')
        //     printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
        // else if (outcome == 'n')
        //     printf("%lx %s\n", addr, "n");          // Print and test if file is read correctly
        
        /*************************************
            Add branch predictor code here
        **************************************/

       if(strcmp(params.bp_name, "bimodal") == 0)            // bimodal branch predictor 
       {

            size_t m;

            m = params.M2;                                      // number of PC bits used to index the bimodal table (M2)
            
            bimodal -> predict(addr,m);                         // start the prediction
            bimodal -> update_branch_table(addr,m,outcome);     // update the bimodal branch table
       }

       if(strcmp(params.bp_name, "gshare") == 0)            // gshare branch predictor 
       {

            size_t m;
            size_t n;

            m = params.M1;                                         // number of PC bits used to index the gshare table (M1)
            n = params.N;                                         // global branch history register bits
            
            gshare -> predict(addr,m,n);                         // start the prediction
            gshare -> update_branch_table(addr,m,n,outcome);     // update the gshare branch table
            gshare -> update_global_history(n,outcome);         // update the global history register after updating the branch table
       }

       if(strcmp(params.bp_name, "hybrid") == 0)             // hybrid branch predictor
        {

            size_t k,m1,n,m2;
            prediction_stats m_stats;

            k = params.K;
            m1 = params.M1;
            n = params.N;
            m2 = params.M2;

            char prediction_hybrid;

            bimodal -> predict(addr,m2);
            char prediction_bimodal = (bimodal -> get_bimodal_prediction());        // get the bimodal prediction
            gshare -> predict(addr,m1,n);
            char prediction_gshare = (gshare -> get_gshare_prediction());           // get the gshare prediction

            if((hybrid -> select_predictor(addr,k,m1,n,m2,outcome)) == 1)         // select gshare predictor
            {

                prediction_hybrid = prediction_gshare;
                gshare -> update_branch_table(addr,m1,n,outcome);
                gshare -> update_global_history(n,outcome);


            }

            else 
            {
                prediction_hybrid = prediction_bimodal;                        // select bimodal predictor
                bimodal -> update_branch_table(addr,m2,outcome);
                gshare -> update_global_history(n,outcome);

            }
            
            hybrid -> predict(outcome, prediction_hybrid);                                          // make a prediction

            hybrid -> update_chooser_table(addr,k,prediction_gshare,prediction_bimodal,outcome);    // update the hybrid chooser table

        }

    }

    if(strcmp(params.bp_name, "bimodal") == 0)
       {

            printf("OUTPUT\n");
            printf(" number of predictions:    %zu\n", bimodal -> m_stats.m_predictions_bimodal);
            printf(" number of mispredictions: %zu\n", bimodal -> m_stats.m_mispredictions_bimodal);
            printf(" misprediction rate:       %0.2f%%\n", (double(bimodal -> m_stats.m_mispredictions_bimodal)/double(bimodal -> m_stats.m_predictions_bimodal)*100));
            
            bimodal -> print_bimodal_contents(params.M2);
       }

    if(strcmp(params.bp_name, "gshare") == 0)
       {

            printf("OUTPUT\n");
            printf(" number of predictions:    %zu\n", gshare -> m_stats.m_predictions_gshare);
            printf(" number of mispredictions: %zu\n", gshare -> m_stats.m_mispredictions_gshare);
            printf(" misprediction rate:       %0.2f%%\n", (double(gshare -> m_stats.m_mispredictions_gshare)/double(gshare -> m_stats.m_predictions_gshare)*100));
            
           gshare -> print_gshare_contents(params.M1);
       }

    if(strcmp(params.bp_name, "hybrid") == 0)
       {

            printf("OUTPUT\n");
            printf(" number of predictions:    %zu\n", hybrid -> m_stats.m_predictions_hybrid);
            printf(" number of mispredictions: %zu\n", hybrid -> m_stats.m_mispredictions_hybrid);
            printf(" misprediction rate:       %0.2f%%\n", (double(hybrid -> m_stats.m_mispredictions_hybrid)/double(hybrid -> m_stats.m_predictions_hybrid)*100));   

            hybrid -> print_hybrid_contents(params.K);
            gshare -> print_gshare_contents(params.M1);
            bimodal -> print_bimodal_contents(params.M2);

       }

    delete bimodal;
    delete gshare;
    delete hybrid;
    
    return 0;
}
