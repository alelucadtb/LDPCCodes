#ifndef CHANNEL_H
#define CHANNEL_H

#include "word.h"
#include "pam.h"
#include "AWGN.h"

class Channel {
    public:
    
    /*Constructor*/
    Channel(AWGN& awgn);
    Channel();

    /*Destructor*/
    ~Channel();

    /**
     * Representation of the AWGN channel
     * @param word: the word to be transmitted
     * @return the word after passing through the channel
    */
    std::vector<double> AWGNChannel(std::vector<int>& word);

    std::pair<std::vector<double>, double> markovChannel(std::vector<double>& word);

    std::tuple<std::vector<double>, double, std::vector<double>> storeMarkovChannel(std::vector<double>& word);

    private:

    AWGN awgn;

};

#endif // CHANNEL_H