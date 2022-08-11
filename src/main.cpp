#include <iostream>
#include <signal.h>
#include "daemon.hpp"
#include "utils/parser.hpp"
#include "utils/config.hpp"

server::Daemon* daem = NULL;
utils::Parser parser("config.yaml");

void terminateSigHandler (int) {
    if(daem != NULL)
        daem->kill ();
    exit (0);
}

int main () {
    parser.parse();
    daem = new server::Daemon();
    signal(SIGINT, &terminateSigHandler);
    daem->start ();
}
