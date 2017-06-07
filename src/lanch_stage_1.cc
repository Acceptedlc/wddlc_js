#include <string>
#include <iostream>
#include "include/libplatform/libplatform.h"
#include "include/v8.h"

using namespace v8;
std::string MainSource(){
    using namespace std;
    string bootstrap_js_code = "(function(process){process.log('hello world');})";
    return bootstrap_js_code; 
}

int main(int argc, char *argv[]) {
    V8::InitializeICUDefaultLocation(argv[0]);
    V8::InitializeExternalStartupData(argv[0]);
    Platform* platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();

    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate* isolate = Isolate::New(create_params); 
    {
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);
    }
    return 0;
}

