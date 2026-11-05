#include <cstdlib>
#include <iostream>
#include <libusb.h>

extern "C" {
int __wrap_libusb_init(libusb_context**);
void __wrap_libusb_exit(libusb_context*);
int __wrap_libusb_handle_events_timeout(libusb_context*, timeval*);
void __wrap_libusb_interrupt_event_handler(libusb_context*);
}

namespace {
int context_token;
int other_context_token;
auto* const sdk_context = reinterpret_cast<libusb_context*>(&context_token);
auto* const other_context = reinterpret_cast<libusb_context*>(&other_context_token);
libusb_context* handled_context = nullptr;
libusb_context* exited_context = nullptr;
libusb_context* interrupted_context = nullptr;
timeval* handled_timeout = nullptr;
int init_result = LIBUSB_SUCCESS;
int event_calls = 0;
int interrupt_calls = 0;

void check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

extern "C" int __real_libusb_init(libusb_context** context)
{
    if (init_result == LIBUSB_SUCCESS && context != nullptr) {
        *context = sdk_context;
    }
    return init_result;
}

extern "C" void __real_libusb_exit(libusb_context* context)
{
    exited_context = context;
}

extern "C" int __real_libusb_handle_events_timeout(libusb_context* context, timeval* timeout)
{
    ++event_calls;
    handled_context = context;
    handled_timeout = timeout;
    return LIBUSB_ERROR_INTERRUPTED;
}

extern "C" void __real_libusb_interrupt_event_handler(libusb_context* context)
{
    ++interrupt_calls;
    interrupted_context = context;
}

int main()
{
    timeval timeout{0, 1000};
    check(__wrap_libusb_handle_events_timeout(nullptr, &timeout) == LIBUSB_ERROR_NO_DEVICE,
          "Events before SDK initialization must not access a default context");
    check(event_calls == 0, "Uninitialized event processing reached libusb");
    __wrap_libusb_interrupt_event_handler(nullptr);
    check(interrupt_calls == 0, "Uninitialized event interrupt reached libusb");

    libusb_context* context = nullptr;
    check(__wrap_libusb_init(&context) == LIBUSB_SUCCESS && context == sdk_context,
          "SDK context initialization failed");
    check(__wrap_libusb_handle_events_timeout(nullptr, &timeout) == LIBUSB_ERROR_INTERRUPTED,
          "Event result must be preserved");
    check(handled_context == sdk_context && handled_timeout == &timeout,
          "SDK events must use the initialized private context and original timeout");
    __wrap_libusb_interrupt_event_handler(nullptr);
    check(interrupted_context == sdk_context,
          "Shutdown must interrupt the same context that handles SDK events");
    __wrap_libusb_interrupt_event_handler(other_context);
    check(interrupted_context == other_context, "Explicit interrupt contexts must be preserved");

    __wrap_libusb_handle_events_timeout(other_context, &timeout);
    check(handled_context == other_context, "Explicit event contexts must be preserved");
    __wrap_libusb_exit(other_context);
    check(exited_context == other_context, "Explicit exit context must be preserved");
    __wrap_libusb_handle_events_timeout(nullptr, &timeout);
    check(handled_context == sdk_context, "Exiting another context cleared the SDK context");

    __wrap_libusb_exit(context);
    check(exited_context == sdk_context, "SDK exit must release the original context");
    check(__wrap_libusb_handle_events_timeout(nullptr, &timeout) == LIBUSB_ERROR_NO_DEVICE,
          "Events after SDK shutdown must not reuse a freed context");
    const int previous_interrupt_calls = interrupt_calls;
    __wrap_libusb_interrupt_event_handler(nullptr);
    check(interrupt_calls == previous_interrupt_calls,
          "Shutdown interrupt must not reuse a freed context");

    init_result = LIBUSB_ERROR_IO;
    check(__wrap_libusb_init(&context) == LIBUSB_ERROR_IO, "Initialization error was lost");
    check(__wrap_libusb_handle_events_timeout(nullptr, &timeout) == LIBUSB_ERROR_NO_DEVICE,
          "Failed initialization must not retain a stale output context");

    init_result = LIBUSB_SUCCESS;
    check(__wrap_libusb_init(&context) == LIBUSB_SUCCESS, "SDK reinitialization failed");
    __wrap_libusb_handle_events_timeout(nullptr, &timeout);
    check(handled_context == sdk_context, "Reinitialized SDK events used the wrong context");
    __wrap_libusb_exit(context);
    std::cout << "SDK USB context checks passed\n";
}
