#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_bp.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <iomanip>

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
using namespace std;
class Bimodal
{
public:
    long unsigned int m;
    int program_counter_length;
    int *program_counter;
    int branch_taken;
    int branch_not_taken;
    int mispredictions;
    float mispredictions_rate;

    Bimodal(long unsigned int m) : m(m)
    {
        program_counter_length = pow(2, m);
        program_counter = new int[program_counter_length];
        for (int i = 0; i < program_counter_length; i++)
        {
            program_counter[i] = 2; // program counter initialized to weakly taken
        }
    }

    uint32_t calc_index(uint32_t address)
    {
        uint32_t mask = (1 << m) - 1; // mask with 'm' bits set to 1
        uint32_t index = (address >> 2) & mask;
        return index;
    }

    int predict(uint32_t index, char outcome)
    {
        int prediction = program_counter[index];
        return prediction;
    }

    int prediction_called_from_hybrid(uint32_t index)
    {
        // uint32_t index = calc_index(address);
        int prediction = program_counter[index];
        return prediction;
    }
    void bimodal_update(int prediction, uint32_t index, char outcome)
    {
        if (outcome == 't' && prediction < 2)
        {
            mispredictions++;
        }
        else if (outcome == 'n' && prediction >= 2)
        {
            mispredictions++;
        }

        if (outcome == 't')
        {
            branch_taken++;
        }
        else
        {
            branch_not_taken++;
        }
        if (outcome == 't' && program_counter[index] < 3)
        {
            program_counter[index]++;
        }
        else if (outcome == 'n' && program_counter[index] > 0)
        {
            program_counter[index]--;
        }
        // cout << "BU: " << index << " " << program_counter[index] << endl;
    }

    // void decrement(uint32_t index)
    // {
    //     if (program_counter[index] > 0)
    //         program_counter[index]--;
    // }

    // void increment(uint32_t index)
    // {
    //     if (program_counter[index] < 3)
    //         program_counter[index]++;
    // }

    void taken(uint32_t address, char outcome)
    {
        uint32_t index = calc_index(address);
        // cout << "index: " << index << endl;
        // predict(index, outcome);
        int prediction = predict(index, outcome);
        bimodal_update(prediction, index, outcome);
        // increment(index);
    }

    void not_taken(uint32_t address, char outcome)
    {
        uint32_t index = calc_index(address);
        // cout << "index: " << index << endl;

        // predict(index, outcome);
        int prediction = predict(index, outcome);
        bimodal_update(prediction, index, outcome);
        // decrement(index);
    }
    void bimodal_stats()
    {
        // mispredictions_rate = (float)mispredictions/(branch_not_taken+branch_taken);
        float mispredictions_rate = static_cast<float>(mispredictions) / static_cast<float>(branch_not_taken + branch_taken);
        mispredictions_rate *= 100;
        cout << "OUTPUT" << endl;
        cout << "number of predictions : " << branch_not_taken + branch_taken << endl;
        cout << "number of mispredictions : " << fixed << setprecision(2) << mispredictions << endl;
        cout << "misprediction rate : " << mispredictions_rate << "%" << endl;
        cout << "FINAL BIMODAL CONTENTS" << endl;
    }
    void print_program_counter()
    {
        for (int i = 0; i < program_counter_length; i++)
        {
            cout << i << " " << program_counter[i] << " " << endl;
        }
    }
};

class Gshare
{
public:
    long unsigned int n;
    long unsigned int m;
    uint32_t m_bits;
    uint32_t n_bits;
    uint32_t BHR;
    uint32_t xor_bits;
    int program_counter_length;
    int *program_counter;
    int branch_taken;
    int branch_not_taken;
    int mispredictions;

    Gshare(long unsigned int n, long unsigned int m) : n(n), m(m)
    {
        program_counter_length = pow(2, m);
        program_counter = new int[program_counter_length];
        BHR = 0; // Initialize BHR to 0
        

        for (int i = 0; i < program_counter_length; i++)
        {
            program_counter[i] = 2; // Initialize program counter entries to weakly taken
        }
    }

    uint32_t calc_index(uint32_t address)
    {
        uint32_t mask = (1 << m) - 1; // mask with 'm' bits set to 1
        uint32_t index = (address >> 2) & mask;
        return index;
    }

    uint32_t uppermost_n_bits_of_m(int m_bits)
    {
        if (n <= 0 || n >= 32)
        {
            return m_bits; // No bits or all bits are requested
        }
        uint32_t mask = (1u << n) - 1; // mask to extract the top n bits
        uint32_t uppermost_n_bits = (m_bits >> (m - n)) & mask;

        // uint32_t lower_bits = m_bits & ((1u << (m - n)) - 1);

        // cout << "value of m_bits: " << dec << m_bits << endl;
        // cout << "mask: " << mask << endl;
        // cout << "value of n: " << n << endl;
        // cout << "uppermost_n_bits_of_m: " << uppermost_n_bits << endl;
        // cout << "lower_bits_of_m: " << lower_bits << endl;
        return uppermost_n_bits;
    }
    uint32_t lower_bits_of_m(int m_bits)
    {
        if (n <= 0 || n >= 32)
        {
            return m_bits; // No bits or all bits are requested
        }
        uint32_t mask = (1u << n) - 1; // mask to extract the top n bits
        uint32_t lower_bits = m_bits & ((1u << (m - n)) - 1);
        // cout << "lower_bits_of_m: " << lower_bits << endl;
        return lower_bits;
    }
    uint32_t bhr_xor_uppermost_n_bits_of_m(int uppermost_n_bits)
    {
        int BHR_xored = BHR ^ uppermost_n_bits;
        // cout << "xor op: " << BHR_xored << endl;
        return BHR_xored;
    }
    uint32_t concatenate_bits(uint32_t BHR_xored, uint32_t lower_bits)
    {
        BHR_xored <<= (m - n);
        BHR_xored |= lower_bits;
        // cout << "Final index: " << BHR_xored << endl;
        return BHR_xored;
    }
    int predict(uint32_t index)
    {
        int prediction = program_counter[index];
        return prediction;
    }
    void gshare_update(int prediction, uint32_t index, char outcome)
    {
        if (outcome == 't' && prediction < 2)
        {
            mispredictions++;
        }
        else if (outcome == 'n' && prediction >= 2)
        {
            mispredictions++;
        }

        if (outcome == 't')
        {
            branch_taken++;
        }
        else
        {
            branch_not_taken++;
        }
        if (outcome == 't')
            increment(index);
        else if (outcome == 'n')
            decrement(index);
        // cout << "GU: " << index << " " << program_counter[index] << endl;
    }
    void decrement(uint32_t index)
    {
        if (program_counter[index] > 0)
            program_counter[index]--;
    }

    void increment(uint32_t index)
    {
        if (program_counter[index] < 3)
            program_counter[index]++;
    }
    int calc_index_for_hybrid(uint32_t m_bits)
    {
        int uppermost_n_bits = uppermost_n_bits_of_m(m_bits);
        int lower_bits = lower_bits_of_m(m_bits);
        int top_m_n_bits = bhr_xor_uppermost_n_bits_of_m(uppermost_n_bits);
        int final_index = concatenate_bits(top_m_n_bits, lower_bits);
        return final_index;
    }
    int prediction_called_from_hybrid(uint32_t index)
    {
        // uint32_t m_bits = calc_index(address);
        // int uppermost_n_bits = uppermost_n_bits_of_m(m_bits);
        // int lower_bits = lower_bits_of_m(m_bits);
        // int top_m_n_bits = bhr_xor_uppermost_n_bits_of_m(uppermost_n_bits);
        // int index = concatenate_bits(top_m_n_bits, lower_bits);
        // predict(index, outcome);
        // increment(index);

        int prediction = predict(index);
        // cout << "GP: " << index << " " << prediction << endl;

        return prediction;
    }
    void taken(uint32_t address, char outcome)
    {

        // printf("%0d is the value of n",n);
        uint32_t index;
        // cout<<"control in taken: "<<hex<<address<<" "<<"outcome: "<<outcome<<endl;
        // cout << "address: " << hex << address << endl;
        if (n != 0)
        {
            // cout<<"n = "<<n<<endl;
            uint32_t m_bits = calc_index(address);
            int uppermost_n_bits = uppermost_n_bits_of_m(m_bits);
            int lower_bits = lower_bits_of_m(m_bits);
            int top_m_n_bits = bhr_xor_uppermost_n_bits_of_m(uppermost_n_bits);
            index = concatenate_bits(top_m_n_bits, lower_bits);
            int prediction = predict(index);
            gshare_update(prediction, index, outcome);
            update_BHR(outcome);
        }
        else if (n==0)
        {
            // cout<<"n = "<<n<<endl;
            index = calc_index(address);
            int prediction = predict(index);
            gshare_update(prediction, index, outcome);
            // predict(index, outcome);
            // increment(index);
            // update_BHR(outcome);
        }
        // int prediction = predict(index);
        // gshare_update(prediction, index, outcome);

        // predict(index, outcome);
        // increment(index);
        // update_BHR(outcome);
    }

    void not_taken(uint32_t address, char outcome)
    {

        // cout<<"control in not taken: "<<hex<<address<<" "<<"outcome: "<<outcome<<endl
        // cout << "address: " << hex << address << endl
        uint32_t index;

        if (n != 0)
        {
            // cout<<"n = "<<n<<endl;

            // uint32_t m_bits = calc_index(address);
            int uppermost_n_bits = uppermost_n_bits_of_m(m_bits);
            int lower_bits = lower_bits_of_m(m_bits);
            int top_m_n_bits = bhr_xor_uppermost_n_bits_of_m(uppermost_n_bits);
            index = concatenate_bits(top_m_n_bits, lower_bits);
            int prediction = predict(index);
            gshare_update(prediction, index, outcome);

            // predict(index, outcome);
            // increment(index);
            update_BHR(outcome);
        }
        else
        {
            // cout<<"n = "<<n<<endl;
            // cout<<"control here"<<endl;
            index = calc_index(address);
            int prediction = predict(index);
            gshare_update(prediction, index, outcome);
            // predict(index, outcome);
            // increment(index);
            // update_BHR(outcome);
        }
    }
    void update_BHR(char outcome)
    {
        BHR >>= 1;
        if (outcome == 't')
        {
            BHR = BHR | (1u << (n - 1));
        }
        else
        {
            BHR = BHR & ~(1u << (n - 1));
        }
    }
    void gshare_stats()
    {
        // mispredictions_rate = (float)mispredictions/(branch_not_taken+branch_taken);
        float mispredictions_rate = static_cast<float>(mispredictions) / static_cast<float>(branch_not_taken + branch_taken);
        mispredictions_rate *= 100;
        cout << "OUTPUT" << endl;
        cout << "number of predictions : " << branch_not_taken + branch_taken << endl;
        cout << "number of mispredictions : " << fixed << setprecision(2) << mispredictions << endl;
        cout << "misprediction rate : " << mispredictions_rate << "%" << endl;
        cout << "FINAL GSHARE CONTENTS" << endl;
    }
    void print_program_counter()
    {
        for (int i = 0; i < program_counter_length; i++)
        {
            cout << i << " " << program_counter[i] << " " << endl;
        }
        // cout << endl;
    }
    ~Gshare()
    {
        delete[] program_counter;
    }
};
class Hybrid
{
public:
    long unsigned int k; // Chooser table index size
    Bimodal bimodal_predictor;
    Gshare gshare_predictor;
    int *chooser_table;

    int branch_taken;
    int branch_not_taken;
    int mispredictions;

    Hybrid(long unsigned int k, long unsigned int m1, long unsigned int m2, long unsigned int n)
        : k(k), bimodal_predictor(m2), gshare_predictor(n, m1)
    {
        chooser_table = new int[1 << k];
        for (int i = 0; i < (1 << k); i++)
        {
            chooser_table[i] = 1;
        }
    }
    uint32_t calc_index(uint32_t address)
    {
        uint32_t mask = (1 << k) - 1;
        uint32_t index = (address >> 2) & mask;
        return index;
    }
    int predict(uint32_t address, char outcome)
    {
        // TODO: Print P value
        uint32_t gshare_m_bits = gshare_predictor.calc_index(address);
        uint32_t gshare_index = gshare_predictor.calc_index_for_hybrid(gshare_m_bits);

        // cout << "gshare index: " << gshare_index << endl;
        uint32_t bimodal_index = bimodal_predictor.calc_index(address);
        // cout << "bimodal index: " << bimodal_index << endl;
        int bimodal_prediction = bimodal_predictor.prediction_called_from_hybrid(bimodal_index);
        // cout << "bimodal prediction: " << bimodal_prediction << endl;
        int gshare_prediction = gshare_predictor.prediction_called_from_hybrid(gshare_index);
        // cout << "BP: " << bimodal_index << " " << bimodal_prediction << endl;

        // cout << "gshare prediction: " << gshare_prediction << endl;
        uint32_t chooser_index = calc_index(address);

        int chooser_counter = chooser_table[chooser_index];
        // cout << "CP: " << chooser_index << " " << chooser_table[chooser_index] << endl;

        int overall_prediction;
        if (chooser_counter >= 2)
        {
            // use gshare

            overall_prediction = gshare_prediction;
            gshare_predictor.gshare_update(overall_prediction, gshare_index, outcome);

            // cout << "chose gshare" << endl;
        }
        else if (chooser_counter <= 1)
        {
            // use bimodal
            overall_prediction = bimodal_prediction;
            bimodal_predictor.bimodal_update(overall_prediction, bimodal_index, outcome);
            // cout << "chose bimodal" << endl;
        }

        gshare_predictor.update_BHR(outcome);
        char gshare_result = (gshare_prediction >= 2) ? 't' : 'n';
        char bimodal_result = (bimodal_prediction >= 2) ? 't' : 'n';

        if ((gshare_result == outcome) && (bimodal_result != outcome))
        {
            if (chooser_table[chooser_index] < 3)
            {
                chooser_table[chooser_index]++;
            }
            // cout << "CU: " << chooser_index << " " << chooser_table[chooser_index] << endl;
        }
        else if ((gshare_result != outcome) && (bimodal_result == outcome))
        {
            if (chooser_table[chooser_index] > 0)
            {
                chooser_table[chooser_index]--;
            }
            // cout << "CU: " << chooser_index << " " << chooser_table[chooser_index] << endl;
        }

        // if ((chooser_table[chooser_index] < 3) && ((((gshare_prediction >= 2) && (outcome == 't')) || ((gshare_prediction <= 1) && (outcome == 'n'))) &&  (((bimodal_prediction >= 2) && (outcome == 'n')) || ((bimodal_prediction <=1)&&(outcome=='t') ))))
        // {
        //     chooser_table[chooser_index]++;
        //     cout << "CU: " << chooser_index << " " << chooser_table[chooser_index] << endl;
        // }
        // else if ((chooser_table[chooser_index] > 0) && (((bimodal_prediction >= 2) && (outcome == 't')) || ((bimodal_prediction <= 1) && (outcome == 'n'))) && ((gshare_prediction >=2)&&(outcome=='n')) || ((gshare_prediction <=1)&&(outcome='t')))
        // {
        //     chooser_table[chooser_index]--;
        //     cout << "CU: " << chooser_index << " " << chooser_table[chooser_index] << endl;
        // }
        return overall_prediction;
    }

    void update_hybrid_stats(int prediction, char outcome)
    {

        if (outcome == 't' && prediction < 2)
        {
            mispredictions++;
        }
        else if (outcome == 'n' && prediction >= 2)
        {
            mispredictions++;
        }

        if (outcome == 't')
        {
            branch_taken++;
        }
        else
        {
            branch_not_taken++;
        }
    }
    void taken(uint32_t address, char outcome)
    {
        int prediction = predict(address, outcome);
        update_hybrid_stats(prediction, outcome);
    }
    void not_taken(uint32_t address, char outcome)
    {
        int prediction = predict(address, outcome);
        update_hybrid_stats(prediction, outcome);
    }
    void hybrid_stats()
    {
        // mispredictions_rate = (float)mispredictions/(branch_not_taken+branch_taken);
        float mispredictions_rate = static_cast<float>(mispredictions) / static_cast<float>(branch_not_taken + branch_taken);
        mispredictions_rate *= 100;
        cout << "OUTPUT" << endl;
        cout << "number of predictions : " << branch_not_taken + branch_taken << endl;
        cout << "number of mispredictions : " << fixed << setprecision(2) << mispredictions << endl;
        cout << "misprediction rate : " << mispredictions_rate << "%" << endl;
        cout << "FINAL CHOOSER CONTENTS" << endl;
    }
    void print_chooser_table()
    {
        for (int i = 0; i < (1 << k); i++)
        {
            cout << i << " " << chooser_table[i] << " " << endl;
        }
        cout << "FINAL GSHARE CONTENTS" << endl;
        gshare_predictor.print_program_counter();
        cout << "FINAL BIMODAL CONTENTS" << endl;
        bimodal_predictor.print_program_counter();
        // cout << endl;
    }
};

int main(int argc, char *argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file

    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    params.bp_name = argv[1];

    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if (strcmp(params.bp_name, "bimodal") == 0) // Bimodal
    {
        if (argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.M2 = strtoul(argv[2], NULL, 10);
        trace_file = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
    }
    else if (strcmp(params.bp_name, "gshare") == 0) // Gshare
    {
        if (argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.M1 = strtoul(argv[2], NULL, 10);
        params.N = strtoul(argv[3], NULL, 10);
        trace_file = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);
    }
    else if (strcmp(params.bp_name, "hybrid") == 0) // Hybrid
    {
        if (argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.K = strtoul(argv[2], NULL, 10);
        params.M1 = strtoul(argv[3], NULL, 10);
        params.N = strtoul(argv[4], NULL, 10);
        params.M2 = strtoul(argv[5], NULL, 10);
        trace_file = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);
    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if (FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    char str[2];

    if (strcmp(params.bp_name, "bimodal") == 0)
    {
        Bimodal *bimodal;
        bimodal = new Bimodal(params.M2);

        while (fscanf(FP, "%lx %s", &addr, str) != EOF)
        {
            outcome = str[0];
            if (outcome == 't')
            {
                bimodal->taken(addr, outcome);
                // printf("%lx %s\n", addr, "t"); // Print and test if file is read correctly
            }
            else if (outcome == 'n')
            {
                bimodal->not_taken(addr, outcome);
            }
            // printf("%lx %s\n", addr, "n"); // Print and test if file is read correctly
        }

        bimodal->bimodal_stats();
        bimodal->print_program_counter();
    }
    else if (strcmp(params.bp_name, "gshare") == 0 )
    {
        // if (params.N != 0)
        // {
        Gshare *gshare;
        gshare = new Gshare(params.N, params.M1);
        // cout<<"Control in else if";

        while (fscanf(FP, "%lx %s", &addr, str) != EOF)
        {
            outcome = str[0];
            if (outcome == 't')
            {
                // cout << "Control in taken";
                gshare->taken(addr, outcome);
                // printf("%lx %s\n", addr, "t"); // Print and test if file is read correctly
            }
            else if (outcome == 'n')
            {
                // cout << "Control in not taken";
                gshare->not_taken(addr, outcome);
                // printf("%lx %s\n", addr, "n"); // Print and test if file is read correctly
            }
        }
        gshare->gshare_stats();
        gshare->print_program_counter();
        // }
    }
    else if (strcmp(params.bp_name, "hybrid") == 0)
    {
        Hybrid *hybrid;
        hybrid = new Hybrid(params.K, params.M1, params.M2, params.N);

        static uint64_t line = 0;
        while (fscanf(FP, "%lx %s", &addr, str) != EOF)
        {
            outcome = str[0];
            // cout << "=" << line++ << " ";

            if (outcome == 't')
            {
                // cout << "Control in taken";
                // printf("%lx %s\n", addr, "t"); // Print and test if file is read correctly

                hybrid->taken(addr, outcome);
            }
            else if (outcome == 'n')
            {
                // cout << "Control in not taken";
                // printf("%lx %s\n", addr, "n"); // Print and test if file is read correctly
                hybrid->not_taken(addr, outcome);
            }
        }
        hybrid->hybrid_stats();
        hybrid->print_chooser_table();
    }

    // bimodal->bimodal_stats();

    /*************************************
        Add branch predictor code here
    **************************************/

    return 0;
}
