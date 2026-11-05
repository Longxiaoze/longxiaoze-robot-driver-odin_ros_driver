#include <atomic>
#include <libusb.h>

// The bundled SDK opens a private context, but its event loop, shutdown drain,
// and shutdown interrupt pass NULL. That selects a different (possibly
// uninitialized) context. Wrap only references linked into this
// executable from the static SDK; calls inside shared libraries are unaffected.
// The driver manages one SDK instance / USB context at a time.
namespace {
std::atomic<libusb_context*> sdk_usb_context{nullptr};
}

extern "C" {
int __real_libusb_init(libusb_context** context);
void __real_libusb_exit(libusb_context* context);
int __real_libusb_handle_events_timeout(libusb_context* context, timeval* timeout);
void __real_libusb_interrupt_event_handler(libusb_context* context);

int __wrap_libusb_init(libusb_context** context)
{
    const int result = __real_libusb_init(context);
    if (result == LIBUSB_SUCCESS && context != nullptr) {
        sdk_usb_context.store(*context);
    }
    return result;
}

void __wrap_libusb_exit(libusb_context* context)
{
    if (context != nullptr) {
        auto* expected = context;
        sdk_usb_context.compare_exchange_strong(expected, nullptr);
    }
    __real_libusb_exit(context);
}

int __wrap_libusb_handle_events_timeout(libusb_context* context, timeval* timeout)
{
    if (context == nullptr) {
        context = sdk_usb_context.load();
        if (context == nullptr) {
            return LIBUSB_ERROR_NO_DEVICE;
        }
    }
    return __real_libusb_handle_events_timeout(context, timeout);
}

void __wrap_libusb_interrupt_event_handler(libusb_context* context)
{
    if (context == nullptr) {
        context = sdk_usb_context.load();
        if (context == nullptr) {
            return;
        }
    }
    __real_libusb_interrupt_event_handler(context);
}
}
