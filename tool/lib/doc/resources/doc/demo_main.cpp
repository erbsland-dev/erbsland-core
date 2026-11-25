// /*{ copyright_line }*/
// /*{ license_line }*/

#include "/*{ header }*/"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
/*% for registration in registrations %*/    app.registerDemo("/*{ registration.name }*/"_el, /*{ registration.function }*/);
/*% endfor %*/    return app.run();
}
