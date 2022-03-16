#include "JavaScriptInterface.hpp"
#include "libplatform/libplatform.h"
#include "include/v8.h"
#include "include/v8-fast-api-calls.h"
#include <cstdio>
#include <map>
#include <stack>
#include <functional>
#include "Engine.hpp"
#include "Types.hpp"
#include "Platform.hpp"
#include "FlintModule.hpp"
#include "RenderElement.hpp"
#include <windows.h>

namespace flint
{
	namespace javascript
	{
       
        class Environment
		{
			friend class Interface;
                
            static wchar_t* toWideChar(const char* utf8)
            {
                const int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
                wchar_t* wstr = new wchar_t [len];
                MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
                return wstr;
            }

            static char* toUTF8(const wchar_t* utf8)
            {
                const int len = WideCharToMultiByte(CP_UTF8, 0, utf8, -1, NULL, 0, 0, 0);
                char* str = new char [len];
                WideCharToMultiByte(CP_UTF8, 0, utf8, -1, str, len, 0, 0);
                return str;
            }

            Environment(Engine& engine) : m_engine(engine), m_pRenderer(nullptr)
			{
            
                //v8::V8::InitializeICUDefaultLocation(argv[0]);
                //v8::V8::InitializeExternalStartupData(argv[0]);

                m_pPlatform = v8::platform::NewDefaultPlatform();
                v8::V8::InitializePlatform(m_pPlatform.get());
                v8::V8::Initialize();
                m_currPath.push(platform::getCWD());
                m_createParams.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
                m_pIsolate = v8::Isolate::New(m_createParams);
                m_pIsolate->SetData(0, this);
				m_pIsolate->Enter();

				v8::HandleScope handle_scope(m_pIsolate);
                v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(m_pIsolate);
                global->Set(m_pIsolate, "print", v8::FunctionTemplate::New(m_pIsolate, print));
                global->Set(m_pIsolate, "setInterval", v8::FunctionTemplate::New(m_pIsolate, setInterval));
                global->Set(m_pIsolate, "setTimeout", v8::FunctionTemplate::New(m_pIsolate, setTimeout));
                global->Set(m_pIsolate, "clearInterval", v8::FunctionTemplate::New(m_pIsolate, removeTimer));
                global->Set(m_pIsolate, "clearTimeout", v8::FunctionTemplate::New(m_pIsolate, removeTimer));

                global->Set(m_pIsolate, "__f_init", v8::FunctionTemplate::New(m_pIsolate, __f_init));
                global->Set(m_pIsolate, "__f_new", v8::FunctionTemplate::New(m_pIsolate, __f_new));
                global->Set(m_pIsolate, "__f_app", v8::FunctionTemplate::New(m_pIsolate, __f_app));
                global->Set(m_pIsolate, "__f_elem", v8::FunctionTemplate::New(m_pIsolate, __f_elem));

                v8::Local<v8::ObjectTemplate> containerTemplate = v8::ObjectTemplate::New(m_pIsolate);
                containerTemplate->SetInternalFieldCount(1);
                m_elemTemplate.Set(m_pIsolate, containerTemplate);

                v8::Local<v8::Context> context = v8::Context::New(m_pIsolate, nullptr, global);
				m_context.Set(m_pIsolate, context);
				context->Enter();
			}

			~Environment()
			{
				m_context.Get(m_pIsolate)->Exit();
				m_pIsolate->Exit();
				m_pIsolate->Dispose();
				v8::V8::Dispose();
				v8::V8::ShutdownPlatform();
				delete m_createParams.array_buffer_allocator;
			}

            v8::MaybeLocal<v8::String> readFile(const wchar_t* name)
            {
                FILE* file = nullptr;
                _wfopen_s(&file, name, L"rb");
                if (file == NULL)
                    return v8::MaybeLocal<v8::String>();

                fseek(file, 0, SEEK_END);
                size_t size = ftell(file);
                rewind(file);

                char* chars = (char*)malloc(size + 1);
                chars[size] = '\0';
                for (size_t i = 0; i < size;) 
                {
                    i += fread(&chars[i], 1, size - i, file);
                    if (ferror(file)) 
                    {
                        fclose(file);
                        return v8::MaybeLocal<v8::String>();
                    }
                }
                fclose(file);
                v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(m_pIsolate, chars, v8::NewStringType::kNormal, static_cast<int>(size));
                free(chars);
                return result;
            }

            void reportException(v8::TryCatch* try_catch)
            {
                v8::HandleScope handle_scope(m_pIsolate);
                v8::String::Utf8Value exception(m_pIsolate, try_catch->Exception());
                v8::Local<v8::Message> message = try_catch->Message();
                if (message.IsEmpty())
                    printf("%s\n", *exception);
                else
                {
                    v8::String::Utf8Value filename(m_pIsolate, message->GetScriptOrigin().ResourceName());
                    v8::Local<v8::Context> context =  m_context.Get(m_pIsolate);
                    const int linenum = message->GetLineNumber(context).FromJust();
                    printf("%s:%i: %s\n", *filename, linenum, *exception);
                    v8::String::Utf8Value sourceline(m_pIsolate, message->GetSourceLine(context).ToLocalChecked());
                    printf("%s\n", *sourceline);
                    const  int start = message->GetStartColumn(context).FromJust();
                    for (int i = 0; i < start; i++)
                        printf(" ");
                    const int end = message->GetEndColumn(context).FromJust();
                    for (int i = start; i < end; i++)
                        printf("^");
                    printf("\n");
                    v8::Local<v8::Value> stack_trace_string;
                    if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) && stack_trace_string->IsString() && stack_trace_string.As<v8::String>()->Length() > 0)
                    {
                        v8::String::Utf8Value stack_trace(m_pIsolate, stack_trace_string);
                        printf("%s\n", *stack_trace);
                    }
                }
            }

            static v8::MaybeLocal<v8::Module> resolveModules(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer)
            {
                v8::Isolate* isolate = context->GetIsolate();
                std::string path(specifier->Length(), 0);
                specifier->WriteOneByte(isolate, (uint8_t*)&path[0]);
                Environment* m_pEnvironment = reinterpret_cast<Environment*>(isolate->GetData(0));
                auto itr = m_pEnvironment->Modules.find(path);
                if (itr != m_pEnvironment->Modules.end())
                {
                    if (itr->second.Get(isolate)->GetStatus() == v8::Module::Status::kEvaluated)
                        return v8::MaybeLocal<v8::Module>(itr->second.Get(isolate));
                    else
                    {
                        const std::string error = "Circular dependancy detected while loading " + path;
                        isolate->ThrowException(v8::String::NewFromUtf8(isolate, error.c_str(), v8::NewStringType::kNormal, (int)error.size()).ToLocalChecked());
                        return v8::MaybeLocal<v8::Module>();
                    }
                }
                else
                {
                    std::unique_ptr<wchar_t> wpath(toWideChar(path.c_str()));
                    return m_pEnvironment->load(wpath.get());
                }
            };

            void execute(const char* code)
            {
                v8::Isolate::Scope isolate_scope(m_pIsolate);
                v8::HandleScope handle_scope(m_pIsolate);
                v8::Local<v8::Context> context = v8::Context::New(m_pIsolate);
                v8::Context::Scope context_scope(context);
                v8::Local<v8::String> source = v8::String::NewFromUtf8(m_pIsolate, code, v8::NewStringType::kNormal).ToLocalChecked();
                v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();
                // Run the script to get the result.
                v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
                if (result.IsEmpty())
                    int k = 0;
            }

            v8::MaybeLocal<v8::Module> load(const wchar_t* moduleName, const char* code = 0)
            {
                v8::EscapableHandleScope handle_scope(m_pIsolate);
                v8::TryCatch try_catch(m_pIsolate);
                v8::Local<v8::String> source;
                std::wstring moduleNameFull;
                std::wstring modulePathFull;
                bool bLoaded = true;
                if (code)
                    v8::String::NewFromUtf8(m_pIsolate, code).ToLocal(&source);
                else if(0 == _wcsicmp(moduleName, L"flint"))
                    v8::String::NewFromUtf8(m_pIsolate, FlintModule).ToLocal(&source);
                else
                {
                    modulePathFull = m_currPath.top();
                    modulePathFull.push_back('\\');
                    modulePathFull += moduleName;
                    modulePathFull = platform::getFullPath(modulePathFull.c_str());
                    moduleNameFull = modulePathFull + L"\\" + platform::getFileName(moduleName);
                    v8::MaybeLocal<v8::String> code = readFile(moduleNameFull.c_str());
                    bLoaded = code.ToLocal(&source);
                }
                if (bLoaded)
                {
                    std::unique_ptr<char> filename(toUTF8(moduleNameFull.c_str()));
                    v8::Local<v8::String> name = v8::String::NewFromUtf8(m_pIsolate, filename.get()).ToLocalChecked();
                    v8::ScriptOrigin origin(m_pIsolate, name, 0, 0, false, -1, v8::Local<v8::Value>(), false, false, true);
                    v8::Local<v8::Context> context = m_context.Get(m_pIsolate);
                    v8::Local<v8::Module> module;
                    v8::ScriptCompiler::Source compiler(source, origin);
                    if (!v8::ScriptCompiler::CompileModule(m_pIsolate, &compiler).ToLocal(&module))
                        reportException(&try_catch);
                    else
                    {
                        auto itr = Modules.insert(std::make_pair(filename.get(), v8::Eternal<v8::Module>(m_pIsolate, module))).first;
                        m_currPath.push(modulePathFull);
                        if (module->InstantiateModule(context, resolveModules).IsNothing())
                        {
                            Modules.erase(itr);
                            m_currPath.pop();
                            reportException(&try_catch);
                        }
                        else
                        {
                            m_currPath.pop();
                            v8::Local<v8::Promise> promise = module->Evaluate(context).ToLocalChecked().As<v8::Promise>();
                            if (v8::Module::kEvaluated == module->GetStatus() && promise->State() == v8::Promise::kFulfilled)
                            {
                                if (moduleName[0] == 0)
                                {
                                    v8::Local<v8::Object> ns = module->GetModuleNamespace()->ToObject(context).ToLocalChecked();
                                    v8::Local<v8::Value> result = ns->Get(context, v8::String::NewFromUtf8Literal(m_pIsolate, "res")).ToLocalChecked();
                                    if (result->IsObject())
                                    {
                                        v8::Local<v8::Object> obj = result->ToObject(context).ToLocalChecked();
                                        v8::Local<v8::Value> processFunc = obj->Get(context, v8::String::NewFromUtf8Literal(m_pIsolate, "update")).ToLocalChecked();
                                        if (processFunc->IsFunction())
                                        {
                                            Application.Set(m_pIsolate, obj);
                                            ProcessFunc.Set(m_pIsolate, v8::Local<v8::Function>::Cast(processFunc));
                                        }
                                    }
                                }
                                return handle_scope.EscapeMaybe(v8::MaybeLocal<v8::Module>(module));
                            }
                            else
                            {
                                Modules.erase(itr);
                                reportException(&try_catch);
                            }
                        }
                    }
                }
                return v8::MaybeLocal<v8::Module>();
            }

            static void getSize(v8::Isolate* pIsolate, v8::Local<v8::Context> context, v8::Local<v8::Object>& params, Size& size)
            {
                v8::Local<v8::Value> value;
                if (params->GetRealNamedProperty(context, v8::String::NewFromUtf8Literal(pIsolate, "size")).ToLocal(&value))
                {
                    int32_t width = size.width, height = size.height;
                    if (value->IsArray())
                    {
                        v8::Local<v8::Array> szArray = v8::Handle<v8::Array>::Cast(value);
                        if (szArray->Length() > 1)
                        {
                            width = szArray->Get(context, 0).ToLocalChecked()->Int32Value(context).ToChecked();
                            height = szArray->Get(context, 1).ToLocalChecked()->Int32Value(context).ToChecked();
                        }
                    }
                    else if (value->IsObject())
                    {
                        v8::Local<v8::Object> szObject = v8::Handle<v8::Object>::Cast(value);
                        auto w = v8::String::NewFromUtf8Literal(pIsolate, "width");
                        auto h = v8::String::NewFromUtf8Literal(pIsolate, "height");
                        if (szObject->Has(context, w).IsJust() && szObject->Has(context, h).IsJust())
                        {
                            width = szObject->Get(context, w).ToLocalChecked()->Int32Value(context).ToChecked();
                            height = szObject->Get(context, h).ToLocalChecked()->Int32Value(context).ToChecked();
                        }
                    }
                    size.width = (Size::Type)std::max<int32_t>(width, 0);
                    size.height = (Size::Type)std::max<int32_t>(height, 0);
                }
            }

            static void getPosition(v8::Isolate* pIsolate, v8::Local<v8::Context> context, v8::Local<v8::Object>& params, Position& position)
            {
                v8::Local<v8::Value> value;
                if (params->GetRealNamedProperty(context, v8::String::NewFromUtf8Literal(pIsolate, "position")).ToLocal(&value))
                {
                    if (value->IsArray())
                    {
                        v8::Local<v8::Array> szArray = v8::Handle<v8::Array>::Cast(value);
                        if (szArray->Length() > 1)
                        {
                            position.x = (Position::Type)szArray->Get(context, 0).ToLocalChecked()->Int32Value(context).ToChecked();
                            position.y = (Position::Type)szArray->Get(context, 1).ToLocalChecked()->Int32Value(context).ToChecked();
                        }
                    }
                    else if (value->IsObject())
                    {
                        v8::Local<v8::Object> szPosition = v8::Handle<v8::Object>::Cast(value);
                        auto x = v8::String::NewFromUtf8Literal(pIsolate, "x");
                        auto y = v8::String::NewFromUtf8Literal(pIsolate, "y");
                        if (szPosition->Has(context, x).IsJust() && szPosition->Has(context, y).IsJust())
                        {
                            position.x = (Position::Type)szPosition->Get(context, x).ToLocalChecked()->Int32Value(context).ToChecked();
                            position.y = (Position::Type)szPosition->Get(context, y).ToLocalChecked()->Int32Value(context).ToChecked();
                        }
                    }
                }
            }

            static void __f_init(const v8::FunctionCallbackInfo<v8::Value>& args)
            {
                v8::Isolate* pIsolate = args.GetIsolate();
                Environment* m_pEnvironment = reinterpret_cast<Environment*> (pIsolate->GetData(0));
                Engine& engine = m_pEnvironment->m_engine;
                v8::HandleScope scope(pIsolate);
                v8::Local<v8::Context> context = pIsolate->GetCurrentContext();
                EngineParameters& engineParams = engine.getParameters();
                if (args.Length() > 0 && args[0]->IsObject())
                {
                    v8::Local<v8::Object> params = args[0]->ToObject(context).ToLocalChecked();
                    getSize(pIsolate, context, params, engineParams.size);
                    v8::Local<v8::Value> value;
                    if (params->GetRealNamedProperty(context, v8::String::NewFromUtf8Literal(pIsolate, "title")).ToLocal(&value))
                    {
                        v8::String::Utf8Value val(pIsolate, value->ToString(context).ToLocalChecked());
                        std::unique_ptr<wchar_t> str(toWideChar(*val));
                        engineParams.title = str.get();
                    }
                    if (params->GetRealNamedProperty(context, v8::String::NewFromUtf8Literal(pIsolate, "state")).ToLocal(&value))
                    {
                        const uint32_t state = value->Int32Value(context).FromJust();
                        if (state <= (int)EngineState::MAXIMIZED)
                            engineParams.state = (EngineState)state;
                    }
                }
                v8::Local<v8::Array> result = v8::Array::New(pIsolate, 4);
                result->Set(context, 0, v8::Integer::NewFromUnsigned(pIsolate, engineParams.size.width));
                result->Set(context, 1, v8::Integer::NewFromUnsigned(pIsolate, engineParams.size.height));
                result->Set(context, 2, v8::String::NewFromTwoByte(pIsolate, (const uint16_t*)engineParams.title.c_str()).ToLocalChecked());
                result->Set(context, 3, v8::Integer::NewFromUnsigned(pIsolate, (uint32_t)engineParams.state));
                args.GetReturnValue().Set(result);
            }

            static void __f_app(const v8::FunctionCallbackInfo<v8::Value>& args)
            {
                v8::Isolate* pIsolate = args.GetIsolate();
                Environment* m_pEnvironment = reinterpret_cast<Environment*> (pIsolate->GetData(0));
                Engine& engine = m_pEnvironment->m_engine;
                v8::HandleScope scope(pIsolate);
                v8::Local<v8::Context> context = pIsolate->GetCurrentContext();
                const uint32_t type = args[0]->Int32Value(context).FromJust();
                switch (type)
                {
                case 0:
                {
                    v8::String::Utf8Value value(pIsolate, args[1]->ToString(context).ToLocalChecked());
                    std::unique_ptr<wchar_t> str(toWideChar(*value));
                    engine.setTitle(str.get());
                }
                break;
                case 1:
                {
                    const uint32_t state = args[1]->Uint32Value(context).FromJust();
                    if (state <= (int)EngineState::MAXIMIZED)
                        engine.setState((EngineState)state);
                }
                break;
                case 3:
                {
                    const int32_t width = std::max<int32_t>(args[1]->Int32Value(context).FromJust(), 0);
                    const int32_t height = std::max<int32_t>(args[2]->Int32Value(context).FromJust(), 0);
                    engine.setSize(Size((Size::Type)width, (Size::Type)height));
                }
                break;
                case 4:
                    engine.quit();
                    break;
                case 5:
                    engine.close();
                    break;
                }
            }

            static void __f_elem(const v8::FunctionCallbackInfo<v8::Value>& args)
            {
                v8::Isolate* pIsolate = args.GetIsolate();
                Environment* m_pEnvironment = reinterpret_cast<Environment*> (pIsolate->GetData(0));
                v8::HandleScope scope(pIsolate);
                v8::Local<v8::Context> context = pIsolate->GetCurrentContext();
                const int32_t val = args[0]->Int32Value(context).ToChecked();
                if (!args[1]->IsObject())
                    return;
                v8::Local<v8::External> pObject = v8::Local<v8::External>::Cast(args[1]->ToObject(context).ToLocalChecked()->GetInternalField(0));
                switch (val)
                {
                case 1:
                {
                    void* pChild = v8::Local<v8::External>::Cast(args[2]->ToObject(context).ToLocalChecked()->GetInternalField(0))->Value();
                    reinterpret_cast<IRenderElement*> (pObject->Value())->add((IRenderElement*)pChild);
                    break;
                }
                case 2:
                {
                    reinterpret_cast<IRenderElement*> (pObject->Value())->release();
                    pObject.Clear();
                    break;
                }
                }
            }

            static void __f_new(const v8::FunctionCallbackInfo<v8::Value>& args)
            {
                v8::Isolate* pIsolate = args.GetIsolate();
                Environment* m_pEnvironment = reinterpret_cast<Environment*> (pIsolate->GetData(0));
                v8::HandleScope scope(pIsolate);
                v8::Local<v8::Context> context = pIsolate->GetCurrentContext();
                const int32_t val = args[0]->Int32Value(context).ToChecked();
                switch (val)
                {
                case 1:
                {
                    v8::Local<v8::Object> value = m_pEnvironment->m_elemTemplate.Get(pIsolate)->NewInstance(context).ToLocalChecked();
                    IRenderElement* pElement = new IRenderElement(*m_pEnvironment->m_pRenderer);
                    value->SetInternalField(0, v8::External::New(pIsolate, pElement));
                    args.GetReturnValue().Set(value);
                    if (args[2]->IsObject())
                    {
                        v8::Local<v8::Object> params = args[2]->ToObject(context).ToLocalChecked();
                        Size size = pElement->getSize();
                        getSize(pIsolate, context, params, size);
                        Position position = pElement->getPosition();
                        getPosition(pIsolate, context, params, position);
                        v8::Local<v8::Value> color;
                        pElement->setPosition(position);
                        pElement->setSize(size);
                        if (params->GetRealNamedProperty(context, v8::String::NewFromUtf8Literal(pIsolate, "backgroundColor")).ToLocal(&color))
                            pElement->setBackgroundColor(Color(color->Uint32Value(context).ToChecked()));
                    }
                }
                break;
                }
            }

            static void print(const v8::FunctionCallbackInfo<v8::Value>& args)
			{
				bool first = true;
				for (int i = 0; i < args.Length(); ++i)
				{
					v8::HandleScope handle_scope(args.GetIsolate());
					if (first)
						first = false;
					else
						printf(" ");
					v8::String::Utf8Value str(args.GetIsolate(), args[i]);
                    std::unique_ptr<wchar_t> buffer(toWideChar(*str));
					wprintf(buffer.get());
                }
				printf("\n");
				fflush(stdout);
			}

            static void setInterval(const v8::FunctionCallbackInfo<v8::Value>& args) { setTimer(0, args); }
            static void setTimeout(const v8::FunctionCallbackInfo<v8::Value>& args) { setTimer(1, args); }

            static void setTimer(int type, const v8::FunctionCallbackInfo<v8::Value>& args)
            {
                v8::Isolate* pIsolate = args.GetIsolate();
                Environment* m_pEnvironment = reinterpret_cast<Environment*> (pIsolate->GetData(0));
                v8::HandleScope scope(pIsolate);
                v8::Local<v8::Context> context = m_pEnvironment->m_context.Get(pIsolate);
                v8::Local<v8::Object> application = m_pEnvironment->Application.Get(pIsolate);
                v8::Local<v8::Value> callback = application->Get(context, v8::String::NewFromUtf8Literal(pIsolate, "addTimer")).ToLocalChecked();
                if (callback->IsFunction() && args.Length() > 1)
                {
                    v8::Local<v8::Value> argPtr[3] = { v8::Integer::New(pIsolate, type), args[0], args[1] };
                    v8::MaybeLocal<v8::Value> result = v8::Handle<v8::Function>::Cast(callback)->Call(context, application, 3, argPtr);
                    args.GetReturnValue().Set(result.ToLocalChecked());
                }
            }

            static void removeTimer(const v8::FunctionCallbackInfo<v8::Value>& args)
            {
                v8::Isolate* pIsolate = args.GetIsolate();
                Environment* m_pEnvironment = reinterpret_cast<Environment*> (pIsolate->GetData(0));
                v8::HandleScope scope(pIsolate);
                v8::Local<v8::Context> context = m_pEnvironment->m_context.Get(pIsolate);
                v8::Local<v8::Object> application = m_pEnvironment->Application.Get(pIsolate);
                v8::Local<v8::Value> callback = application->Get(context, v8::String::NewFromUtf8Literal(pIsolate, "removeTimer")).ToLocalChecked();
                if (callback->IsFunction() && args.Length() > 0)
                {
                    v8::Local<v8::Value> argPtr[1] = { args[0] };
                    v8::Handle<v8::Function>::Cast(callback)->Call(context, application, 1, argPtr);
                }
            }

		private:

            v8::Isolate* m_pIsolate;
            v8::Isolate::CreateParams m_createParams;
            v8::Eternal<v8::Context> m_context;
            std::unique_ptr<v8::Platform> m_pPlatform;
			std::map<std::string, v8::Eternal<v8::Module>> Modules;
            std::stack<std::wstring> m_currPath;
            v8::Eternal<v8::ObjectTemplate> m_elemTemplate;
            v8::Eternal<v8::Object> Application;
            v8::Eternal<v8::Function> ProcessFunc;
            Engine&     m_engine;
            Renderer*   m_pRenderer;
		};
	
		Interface::Interface(Engine& engine) : m_engine(engine),
                                               m_pEnvironment(new Environment(m_engine))
		{
		}

		Interface::~Interface()
		{
            delete m_pEnvironment;
		}

		bool Interface::initialize(const wchar_t* main)
		{
            v8::Isolate* pIsolate = m_pEnvironment->m_pIsolate;
            v8::HandleScope scope(pIsolate);
            v8::MaybeLocal<v8::Module> module = m_pEnvironment->load(main);
            if (!module.IsEmpty())
            {
                v8::Local<v8::Context> context = m_pEnvironment->m_context.Get(pIsolate);
                v8::MaybeLocal<v8::Object> global = module.ToLocalChecked()->GetModuleNamespace()->ToObject(context);
                v8::Local<v8::Object> obj;
                if (global.ToLocal(&obj))
                {
                    v8::MaybeLocal<v8::Array> props = obj->GetOwnPropertyNames(context);
                    v8::Local<v8::Array> arr;
                    if (props.ToLocal(&arr) && arr->Length() == 1)
                    {
                        v8::Local<v8::Value> className = arr->Get(context, 0).ToLocalChecked();
                        v8::Local<v8::String> str = className->ToString(context).ToLocalChecked();
                        v8::String::Utf8Value pname(pIsolate, str);
                        const std::string name = *pname;
                        std::unique_ptr<char> wname(Environment::toUTF8(main));
                        const std::string smain = wname.get();
                        const std::string code = (name == "default") ? "import defaultExport from '" + smain + "';\nexport let res=new defaultExport();" : "import {" + name + "} from '" + smain + "';\nexport let res=new " + name + "();";
                        m_pEnvironment->load(L"", code.c_str());
                        return !m_pEnvironment->Application.IsEmpty();
                    }
                }
            }
            return false;
		}

        void Interface::load()
        {
            v8::Isolate* pIsolate = m_pEnvironment->m_pIsolate;
            v8::HandleScope scope(pIsolate);
            v8::Local<v8::Context> context = m_pEnvironment->m_context.Get(pIsolate);
            v8::Local<v8::Object> application = m_pEnvironment->Application.Get(pIsolate);
            v8::Local<v8::Value> callback = application->Get(context, v8::String::NewFromUtf8Literal(pIsolate, "load")).ToLocalChecked();
            m_pEnvironment->m_pRenderer = m_pEnvironment->m_engine.getRenderer();
            v8::Local<v8::Object> value = m_pEnvironment->m_elemTemplate.Get(pIsolate)->NewInstance(context).ToLocalChecked();
            value->SetInternalField(0, v8::External::New(pIsolate, m_pEnvironment->m_pRenderer->getStage() ));
            v8::Local<v8::Value> args[] = { value };
            v8::Handle<v8::Function>::Cast(callback)->Call(context, application, 1, args);
        }

        bool Interface::update(float delta)
        {
            v8::Isolate* pIsolate = m_pEnvironment->m_pIsolate;
            v8::HandleScope scope(pIsolate);
            v8::Local<v8::Context> context = m_pEnvironment->m_context.Get(pIsolate);
            v8::Local<v8::Object> application = m_pEnvironment->Application.Get(pIsolate);
            v8::Local<v8::Function> onProcess = m_pEnvironment->ProcessFunc.Get(pIsolate);
            v8::Local<v8::Value> args[] = { v8::Number::New(pIsolate, delta) };
            onProcess->Call(context, application, 1, args);
            return true;
        }

        bool Interface::beforeUnload()
        {
            v8::Isolate* pIsolate = m_pEnvironment->m_pIsolate;
            v8::HandleScope scope(pIsolate);
            v8::Local<v8::Context> context = m_pEnvironment->m_context.Get(pIsolate);
            v8::Local<v8::Object> application = m_pEnvironment->Application.Get(pIsolate);
            v8::Local<v8::Value> callback = application->Get(context, v8::String::NewFromUtf8Literal(pIsolate, "onbeforeunload")).ToLocalChecked();
            if (callback->IsFunction())
            {
                v8::MaybeLocal<v8::Value> retVal = v8::Handle<v8::Function>::Cast(callback)->Call(context, application, 0, nullptr);
                v8::Local<v8::Value> value;
                return (retVal.ToLocal(&value) && value->BooleanValue(pIsolate));
            }
            return false;
        }

        void Interface::unload()
        {
            v8::Isolate* pIsolate = m_pEnvironment->m_pIsolate;
            v8::HandleScope scope(pIsolate);
            v8::Local<v8::Context> context = m_pEnvironment->m_context.Get(pIsolate);
            v8::Local<v8::Object> application = m_pEnvironment->Application.Get(pIsolate);
            v8::Local<v8::Value> callback = application->Get(context, v8::String::NewFromUtf8Literal(pIsolate, "onunload")).ToLocalChecked();
            if (callback->IsFunction())
                v8::Handle<v8::Function>::Cast(callback)->Call(context, application, 0, nullptr);
        }

        bool Interface::event(uint8_t type, uint8_t a, const wchar_t* b, uint8_t c)
        {
            v8::Isolate* pIsolate = m_pEnvironment->m_pIsolate;
            v8::HandleScope scope(pIsolate);
            v8::Local<v8::Context> context = m_pEnvironment->m_context.Get(pIsolate);
            v8::Local<v8::Object> application = m_pEnvironment->Application.Get(pIsolate);
            v8::Local<v8::Value> callback = application->Get(context, v8::String::NewFromUtf8Literal(pIsolate, "fireEvent")).ToLocalChecked();
            v8::Local<v8::Value> args[4] = { v8::Integer::New(pIsolate, type), v8::Integer::New(pIsolate, a), v8::String::NewFromTwoByte(pIsolate, (const uint16_t*)b).ToLocalChecked(), v8::Integer::New(pIsolate, c) };
            return v8::Handle<v8::Function>::Cast(callback)->Call(context, application, 4, args).ToLocalChecked()->BooleanValue(pIsolate);
        }
     
	}
}