/*! \file main.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 3
 *
 * This file contains the main program for Project 3.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "app.hpp"

int main(int argc, char *argv[])
{
    std::vector<std::string> args(argv, argv + argc);
    Proj3 app(args);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
