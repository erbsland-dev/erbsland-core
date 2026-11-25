/*% extends "headers/header.cpp" %*/
/*% block body %*/#ifdef ERBSLAND_CORE_BUILDING_LIBRARY
#error "Namespace flattening is forbidden when compiling Erbsland Core."
#endif

/*% for path in includes %*/#include "/*{ path }*/"
/*% endfor %*//*% if forwards %*/
#include <cstdint>
/*% endif %*//*% for name, declarations in forwards %*/
namespace /*{ name }*/ {

/*% for declaration in declarations %*//*{ declaration }*/
/*% endfor %*/
}
/*% endfor %*/
namespace /*{ base_namespace }*/ {

/*% for name in imports %*/using namespace ::/*{ name }*/;
/*% endfor %*//*% for name, symbols in exports %*//*% for symbol in symbols %*/using ::/*{ name }*/::/*{ symbol }*/;
/*% endfor %*//*% endfor %*//*% for alias, target in aliases %*/namespace /*{ alias }*/ = ::/*{ target }*/;
/*% endfor %*/
}
/*% endblock %*/
