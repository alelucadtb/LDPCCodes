#include "decoder.h"

Decoder::Decoder(std::vector<double> received_word, Graph& graph, double variance, std::vector<int> modulated_word, PAM& pam) : received_word(received_word), graph(graph), variance(variance), modulated_word(modulated_word), pam(pam) {
    equalityNodesSize = graph.getEqualityNodesSize();
    wNodesSize =  received_word.size();
    adjListWNodes = std::vector<std::vector<std::pair<int, double>>>(wNodesSize);
    adjListEqqNodes = std::vector<std::vector<std::pair<int, double>>>(equalityNodesSize);
}

Decoder::~Decoder() {}

void Decoder::addLink(int src, int dest){
    adjListWNodes[src].push_back({dest, 0});
    adjListEqqNodes[dest].push_back({src, 0});
}

/* Generate the graph between the w-nodes and the equality nodes*/
void Decoder::generateGraph(){
    int bit_per_symbol = log2(pam.getM());
    for(int i = 0; i < wNodesSize; i++){
        for(int j = 0; j < bit_per_symbol; j++){
            /* Add a link between the w-node and the equality node */
            addLink(i, i*bit_per_symbol + j);
        }
    }
}

int Decoder::flipBit(int bit){
    if(bit == 0){
        return 1;
    }
    return 0;
}

int Decoder::mapGrayNumber(std::vector<int> bit){
    std::string bitString;
    /*Convert the bit vector into a string*/
    for (int i = 0; i < bit.size(); ++i) {
        bitString += std::to_string(bit[i]);
    }
    Gray gray;
    int bit_per_symbol = log2(pam.getM());
    std::string gray_total = "";
    /*Generate the gray codes*/
    std::vector<std::string> gray_codes = gray.generateGrayarr(bit_per_symbol);
    /*Generate the gray codes in integer*/
    std::vector<int> gray_codes_int(gray_codes.size());
    for (int i = 0; i < gray_codes.size(); i++) {
        gray_codes_int[i] = 2*(i)-(pam.getM()-1);
    }
    /*Find the corresponding gray number*/
    for(int i = 0; i < gray_codes.size(); i++){
        if(gray_codes[i] == bitString){
            return gray_codes_int[i];
        }
    }
    return -1;
}

std::vector<double> Decoder::calculateLLRwNodes(int output_link, int node_number){
    std::vector<double> LLRwNodes;
    for(int j = 0; j < adjListWNodes[node_number].size(); j++){
        int dest = adjListWNodes[node_number][j].first;
        for(int j_first = 0; j_first < adjListEqqNodes[dest].size(); j_first++){
            int dest_first = adjListEqqNodes[dest][j_first].first;
            if(dest_first == node_number && j != output_link){
                LLRwNodes.push_back(adjListEqqNodes[dest][j_first].second);
            }
        }
    }
    return LLRwNodes;
}

double Decoder::calculateGFunction(int d, int l){
    double temp_value_1 = std::fabs(received_word[l] - d);
    double temp_value_2 = variance;
    return exp(-1.0*temp_value_1*temp_value_1/(2.0*temp_value_2));
}

std::vector<std::vector<int>> Decoder::generatePermutation(){
    int bit = log2(pam.getM()) - 1;
    int num_options = pam.getM() / 2;
    std::vector<std::vector<int>> possible_permutation;
    std::vector<int> bit_permutation;
    std::string binary;
    for(int i = 0; i < num_options; i++){
        binary = std::bitset<8>(i).to_string(); //to binary
        for(int k = binary.length() - bit; k < binary.length(); k++){
            bit_permutation.push_back(binary[k] - '0');
        }
        possible_permutation.push_back(bit_permutation);
        bit_permutation.clear();
    }
    return possible_permutation;
}

std::vector<int> Decoder::fastDecodingCycle(){
    std::vector<double> g;
    std::vector<int> output_link;
    std::vector<std::vector<int>> vector_map_0;
    std::vector<std::vector<int>> vector_map_1;
    std::vector<std::vector<int>> exponent_map_0;
    std::vector<std::vector<int>> exponent_map_1;
    double temp_1;
    std::vector<int> result_vector;
    int bit_per_symbol = log2(pam.getM());
    //Generate the graph in the object of the class Decoder
    generateGraph();

    //Create a temporary vector_map element
    std::vector<int> vector_map_temp (bit_per_symbol, 0);
    std::vector<int> exponent_map_temp (bit_per_symbol-1, 0);
    
    /*Create an array of possible permutation of M-1 bits*/
    std::vector<std::vector<int>> possible_permutation = generatePermutation();
    for(int i = 0; i < bit_per_symbol; i++){
        /* Select the value of the fixed bit in the vector_map */
        for(int i_first = 0; i_first < 2; i_first++){
            vector_map_temp[i] = i_first;
            for(int j = 0; j < possible_permutation.size(); j++){
                int k = 0;
                for(int j_first = 0; j_first < possible_permutation[j].size(); j_first++){
                    /* Flip the bit */
                    exponent_map_temp[j_first] = !possible_permutation[j][j_first];
                    if(k == i){
                        k++;
                    }
                    vector_map_temp[k] =  possible_permutation[j][j_first];
                    k++;
                }
                if(i_first == 0){
                    vector_map_0.push_back(vector_map_temp);
                    exponent_map_0.push_back(exponent_map_temp);
                    output_link.push_back(i);
                }
                else{
                    vector_map_1.push_back(vector_map_temp);
                    exponent_map_1.push_back(exponent_map_temp);
                }
            }
        }
    }


    double num_1;
    double den_1;
    double num_2;
    double den_2;
    double num;
    double den;
    int out = output_link[0];
    std::vector<std::pair<double, double>> temp_vector;

    for(int i = 0; i < received_word.size(); i++){
        //Select one of the possibile combinations of vector_map
        for(int j = 0; j < vector_map_0.size(); j++){
            //Select one of the possibile combinations of exponent_map
            std::vector<double> LLRwNodes = calculateLLRwNodes(output_link[j], i);
            num_1 = calculateGFunction(mapGrayNumber(vector_map_0[j]), i);
            den_1 = calculateGFunction(mapGrayNumber(vector_map_1[j]), i);
            num_2 = den_2 = 0.0;
            for(int k = 0; k < exponent_map_0[j].size(); k++){
                num_2 += ((double)exponent_map_0[j][k]) * LLRwNodes[k];
                den_2 += ((double)exponent_map_1[j][k]) * LLRwNodes[k];  
            }
            num_2 = exp(num_2);
            den_2 = exp(den_2);
            
            if(out != output_link[j]){
                temp_vector.push_back(std::make_pair(num, den));
                out = output_link[j];
                num = num_1 * num_2;
                den = den_1 * den_2;
            } else{
                num += num_1 * num_2;
                den += den_1 * den_2;
            }

        }
        temp_vector.push_back(std::make_pair(num, den));
        out = output_link[0];

        //std::cout << "temp_vector.size(): " << temp_vector.size() << std::endl;
        /*Calculate the LLR for the w-nodes*/
        for(int j = 0; j < temp_vector.size(); j++){
            double LLR_temp = temp_vector[j].first/temp_vector[j].second;
            //std::cout << "LLR_temp: " << LLR_temp << std::endl;
            adjListWNodes[i][j].second = log(LLR_temp);
        }
        //std::cout << "adjListWNodes[i].size(): " << adjListWNodes[i].size() << std::endl;
        temp_vector.clear();
    }

    for(int i = 0; i < adjListWNodes.size(); i++){
        for(int j = 0; j < adjListWNodes[i].size(); j++){   
            //std::cout << "adjListWNodes[" << i << "][" << j << "]: " << adjListWNodes[i][j].second << std::endl;
        }
    }

    graph.generateGraph();
    double LLR_g;
    bool codeword = false;
    int counter = 0;
    std::vector<int> decodedBits(graph.equalityNodesSize);
    double sum_1 = 0;
    double sum = 0;
    /* Vector that contains the sum of the messages that enter in the equality nodes*/
    std::vector<double> vectorEqualityNodes(graph.equalityNodesSize);
   
    /*Messages that leave the equality nodes*/
    while(!codeword){
        /*Messages that leave the equality nodes*/
        for(int i = 0; i < graph.adjListEqualityNodes.size(); i++)
        {
            /*Check all the links connected to the current equality node*/
            for(int j = 0; j < graph.adjListEqualityNodes[i].size(); ++j)
            {   
                sum = 0;
                /*Get the destination of the current link, so the check node*/
                int dest = graph.adjListEqualityNodes[i][j].first;
                /*Sum of the messages coming from the w-nodes to the equality node*/
                for(int l = 0; l < adjListWNodes.size(); l++){
                    for(int z = 0; z < adjListWNodes[l].size(); z++){
                        if(adjListWNodes[l][z].first == i){
                            sum_1 = adjListWNodes[l][z].second;
                            //std::cout << "sum_1: " << sum_1 << std::endl;
                            goto jump;
                        }
                    }
                }
                jump:
                /*Check all the links connected to the current equality node*/
                for(int j_first = 0; j_first < graph.adjListEqualityNodes[i].size(); j_first++){
                    /*If the current link is the same as the one we are checking, skip it*/
                    if(j_first == j){
                        continue;
                    }
                    int dest_first = graph.adjListEqualityNodes[i][j_first].first;
                    /*Check all the links connected to the current check node*/
                    for(int j_second = 0; j_second < graph.adjListCheckNodes[dest_first].size(); j_second++){
                        /*If destination of the current check nodes is the current equality node, add the message to the sum*/
                        if(graph.adjListCheckNodes[dest_first][j_second].first == i){
                            sum += graph.adjListCheckNodes[dest_first][j_second].second;
                            break;
                        }
                    }
                }
                /*Set the message to the current equality node*/
                graph.adjListEqualityNodes[i][j].second = sum + sum_1;
                //std::cout << "value: " << graph.adjListEqualityNodes[i][j].second << std::endl;
            }
        }
        //exit(0);
        /*Sum of the messages that enter in the equality nodes*/
        for(int i = 0; i < graph.adjListEqualityNodes.size(); i++){
            double sum_3 = adjListWNodes[i/bit_per_symbol][i%bit_per_symbol].second;
            for(int j = 0; j < graph.adjListEqualityNodes[i].size(); j++){
                /*Get the destination, so the check node*/
                int dest = graph.adjListEqualityNodes[i][j].first;
                /*Sum of the messages coming from the check nodes*/
                for(int j_first = 0; j_first < graph.adjListCheckNodes[dest].size(); j_first++){
                    if(graph.adjListCheckNodes[dest][j_first].first == i){
                        sum_3 += graph.adjListCheckNodes[dest][j_first].second;
                    }
                }      
            }
            vectorEqualityNodes[i] = sum_3;
        }
        /*Marginalization*/
        for(int i = 0; i < vectorEqualityNodes.size(); i++){
            if(vectorEqualityNodes[i] > 0){
                decodedBits[i] = 0;
            }else{
                decodedBits[i] = 1;
            }
        }
        if(graph.matrix.isCodewordVector(decodedBits)){
            codeword = true;
        }
        
    }

    std::cout << "counter: " << counter << std::endl;
    return decodedBits;

}

