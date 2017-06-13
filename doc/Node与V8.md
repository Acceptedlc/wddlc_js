# Node与V8

Node.js是一个基于Chrome V8 JavaScript engine的JavaScript运行时环境。从另一个角度来说，Node.js使用V8扩展了JavaScript这门语言的边界，让它走出了浏览器，直接可以在操作系统层面运行起来。

__Node是如何使用V8的囊？__ 

*ps：本文中研究的Node.js 的版本是6.10.3*

### 起点：main

main函数位于src/node_main.cc当中
```
int main(int argc, char *argv[]) {
  // Disable stdio buffering, it interacts poorly with printf()
  // calls elsewhere in the program (e.g., any logging from V8.)
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
  return node::Start(argc, argv);
}
```

从代码可以看出，就是调用了node命名空间下的Start函数，没做啥实际的事情

node::Start位于src/node.cc的line 4709

开始为libuv（本文暂不讨论，后面会有专门的文章）和V8准备启动的参数

这里重点关注line 4742
```
StartNodeInstance(&instance_data);
```
从方法名上可以看出，在这个function中正式启动了node实例


### 启动Node实例

这个方法位于src/node.cc的line 4604

对于这个方法重点关注下面的内容
```
Isolate* isolate = Isolate::New(params);//line 4612

Isolate::Scope isolate_scope(isolate); //line 4628
HandleScope handle_scope(isolate);
Local<Context> context = Context::New(isolate);

Context::Scope context_scope(context);//line 4633

LoadEnvironment(env); //line 4651
```
4612行node实例为自己创建了V8 引擎的实例。在V8的官方文档中，说Isolate代表一个独立的V8引擎的实例，不同的Isolate是完全隔离的。

紧接着在，4628进入了这个引擎当中，在其中创建了Context。
关于Content这个概念需要多说几句了。
> In V8, a context is an execution environment that allows separate, unrelated, JavaScript applications to run in a single instance of V8. You must explicitly specify the context in which you want any JavaScript code to be run.

从官方的解释中，我们可以看出Content也是一个沙箱的感觉。只不过与isolate相比是一个更轻的沙箱。为啥需要这个一个东西囊，比如在浏览器中，有一个iframe，我们就可以新建立一个Content，从而复用原有的全局对象方法等。

在4633行进入这个运行环境，然后执行一个很重要的方法LoadEnvironment，在这个方法中从cpp的世界进入了JavaScript当中

### LoadEnvironment

这个方法位于 src/node.cc line 3482

对于这个方法，重点关注下面几行
```
//line 3500
Local<String> script_name = FIXED_ONE_BYTE_STRING(env->isolate(),
                                                    "bootstrap_node.js");
Local<Value> f_value = ExecuteString(env, MainSource(env), script_name);

//line 3508
CHECK(f_value->IsFunction());
  Local<Function> f = Local<Function>::Cast(f_value);

//line 3547
Local<Value> arg = env->process_object();
f->Call(Null(env->isolate()), 1, &arg);
```
f_value的内容就是bootstrap_node.js，这个js文件的结构如下，f_value就是匿名函数在cpp中的对象

```
(function(process){
    function startup(){
      ...
         //line 137
         // read the source
         const filename = Module._resolveFilename(process.argv[1]);                    
         var source = fs.readFileSync(filename, 'utf-8');
         // remove shebang and BOM
         source = internalModule.stripBOM(source.replace(/^#!.*/, ''));
         // wrap it
         source = Module.wrap(source);
         // compile the script, this will throw if it fails
         new vm.Script(source, {filename: filename, displayErrors: true});

        preloadModules();
        run(Module.runMain);
      ...
    }

    startup()
})
```

接下来在3547行，调用了这个方法的Call，从而进入了js的世界。

在js的137行会读取命令行指定的脚本文件，然后运行。

### 总结

node的启动过程分为下面几步
* 初始化V8 Engine
* 编译运行bootstrap_node.js
* 获得一个js functin在V8模型中的实例
* 运行这个js function，在这个function中会读取命令行中指定的脚本并运行

试验小程序：https://github.com/Acceptedlc/wddlc_js/blob/lanch1/src/lanch_stage_1.cc

ps：运行环境为Ubuntu 16.04 x86_64
