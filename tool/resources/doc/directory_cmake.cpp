# /*{ copyright_line }*/
# /*{ license_line }*/

cmake_minimum_required(VERSION 3.28)

/*% for entry in entries %*/add_subdirectory(/*{ entry }*/)
/*% endfor %*/