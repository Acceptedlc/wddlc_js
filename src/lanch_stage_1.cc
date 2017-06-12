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

const char* ToCString(const String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}


static void LogCallback(const v8::FunctionCallbackInfo<v8::Value>& args){
    if (args.Length() < 1) return;
    HandleScope scope(args.GetIsolate());
    Local<Value> arg = args[0];
    String::Utf8Value value(arg);
    const char* name = ToCString(value);
    printf("%s\n",name);
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
        v8::Local<v8::Context> context = Context::New(isolate);
        Context::Scope context_scope(context);

        Local<String> source = String::NewFromUtf8(isolate, MainSource().c_str(), NewStringType::kNormal, MainSource().size()).ToLocalChecked();
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();
        Local<Value> result = script->Run(context).ToLocalChecked();

        Local<FunctionTemplate> process_template = FunctionTemplate::New(isolate);
        process_template->SetClassName(String::NewFromUtf8(isolate, "process", NewStringType::kNormal, sizeof("process") - 1).ToLocalChecked());
        Local<Object> process_object =
            process_template->GetFunction()->NewInstance(context).ToLocalChecked();
        process_object->Set(
                String::NewFromUtf8(isolate, "log", NewStringType::kNormal).ToLocalChecked(),
                FunctionTemplate::New(isolate, LogCallback)->GetFunction()
        );

        Local<Function> f = Local<Function>::Cast(result);
        Local<Value> thisObj = Null(isolate);
        Local<Value> arg = process_object;
        f->Call(Null(isolate), 1, &arg);
    }
    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    delete platform;
    delete create_params.array_buffer_allocator;
    return 0;
}

