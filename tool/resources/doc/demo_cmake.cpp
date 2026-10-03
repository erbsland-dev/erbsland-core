# /*{ copyright_line }*/
# /*{ license_line }*/

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(/*{ target }*/)
target_sources(/*{ target }*/ PRIVATE
/*% for source in sources %*/        /*{ source }*/
/*% endfor %*/)
