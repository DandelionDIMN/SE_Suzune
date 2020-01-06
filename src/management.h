#pragma once
#include "function.h"
#include "filestream.h"

namespace kagami::management {
  using FunctionImplCollection = map<string, FunctionImpl>;
  using FunctionHashMap = unordered_map<string, FunctionImpl *>;

  void CreateImpl(FunctionImpl impl, string domain = kTypeIdNull);
  FunctionImpl *FindFunction(string id, string domain = kTypeIdNull);

  Object *CreateConstantObject(string id, Object &object);
  Object *CreateConstantObject(string id, Object &&object);
  Object GetConstantObject(string id);
}

namespace kagami::management::type {
  const unordered_set<string> RepackableObjTypes = {
    kTypeIdInt, kTypeIdFloat, kTypeIdBool, kTypeIdString,
    kTypeIdWideString, kTypeIdInStream, kTypeIdOutStream
  };

  template <class T>
  bool PlainComparator(Object &lhs, Object &rhs) {
    return lhs.Cast<T>() == rhs.Cast<T>();
  }

  vector<string> GetMethods(string id);
  bool CheckMethod(string func_id, string domain);
  size_t GetHash(Object &obj);
  bool IsHashable(Object &obj);
  bool IsCopyable(Object &obj);
  void CreateObjectTraits(string id, ObjectTraits temp);
  Object CreateObjectCopy(Object &object);
  bool CheckBehavior(Object obj, string method_str);
  bool CompareObjects(Object &lhs, Object &rhs);

  class ObjectTraitsSetup {
  private:
    string type_id_;
    string methods_;
    DeliveryImpl delivering_impl_;
    Comparator comparator_;
    HasherFunction hasher_;
    vector<FunctionImpl> impl_;
    FunctionImpl delivering_;

  public:
    ObjectTraitsSetup() = delete;

    ObjectTraitsSetup(
      string type_name,
      DeliveryImpl dlvy,
      HasherFunction hasher) :
      type_id_(type_name),
      delivering_impl_(dlvy),
      comparator_(nullptr),
      hasher_(hasher) {}

    ObjectTraitsSetup(string type_name, DeliveryImpl dlvy) :
      type_id_(type_name), delivering_impl_(dlvy), hasher_(nullptr) {}

    ObjectTraitsSetup &InitConstructor(FunctionImpl impl) {
      delivering_ = impl; return *this; 
    }

    ObjectTraitsSetup &InitComparator(Comparator comparator) {
      comparator_ = comparator; return *this; 
    }

    ObjectTraitsSetup &InitMethods(initializer_list<FunctionImpl> &&rhs);
    ~ObjectTraitsSetup();
  };
}

namespace kagami::management::script {
  using ProcessedScript = pair<string, VMCode>;
  using ScriptStorage = unordered_map<string, VMCode>;

  VMCode *FindScriptByPath(string path);
  VMCode &AppendScript(string path, VMCode &code);
  VMCode &AppendBlankScript(string path);
}

namespace kagami::management::extension {
#ifdef _WIN32
  using LoadedLibraryUnit = pair<string, HMODULE>;
  using LibraryMgmtStorage = unordered_map<string, HMODULE>;
#else
  using LoadedLibraryUnit = pair<string, void *>;
  using LibraryMgmtStorage = unordered_map<string, void *>;
#endif

#ifdef _WIN32
  class Extension {
  protected:
    HMODULE ptr_;
  public:
    ~Extension() { FreeLibrary(ptr_); }
    Extension() : ptr_(nullptr) {}
    Extension(wstring path) : ptr_(LoadLibrary(path.data())) {}

    template <typename _TargetFunction>
    bool GetTargetInterface(_TargetFunction &func, string id) {
      func = static_cast<_TargetFunction>(GetProcAddress(ptr_, id.data()));
      if (func == nullptr) return false;
      return true;
    }
  };
#else

#endif
  //Callback facilities
  void DisposeMemoryUnit(void *ptr);
  void DisposeMemoryUnitGroup(void *ptr);
  int FetchInt(int64_t **target, void *obj_map, const char *id);
  int FetchFloat(double **target, void *obj_map, const char *id);
  int FetchBool(int **target, void *obj_map, const char *id);
  int FetchString(char **target, void *obj_map, const char *id);
  int FetchWideString(wchar_t **target, void *obj_map, const char *id);
  int FetchInStream(FILE **target, void *obj_map, const char *id);
  int FetchOutStream(FILE **target, void *obj_map, const char *id);
}

namespace kagami::management::runtime {
  void InformBinaryPathAndName(string info);
  string GetBinaryPath();
  string GetBinaryName();
  string GetWorkingDirectory();
  bool SetWorkingDirectory(string dir);
  void InformScriptPath(string path);
  string GetScriptAbsolutePath();
}

namespace std {
  template <>
  struct hash<kagami::Object> {
    size_t operator()(kagami::Object const &rhs) const {
      auto copy = rhs; //solve with limitation
      size_t value = 0;
      if (kagami::management::type::IsHashable(copy)) {
        value = kagami::management::type::GetHash(copy);
      }

      return value;
    }
  };

  template <>
  struct equal_to<kagami::Object> {
    bool operator()(kagami::Object const &lhs, kagami::Object const &rhs) const {
      auto copy_lhs = lhs, copy_rhs = rhs;
      return kagami::management::type::CompareObjects(copy_lhs, copy_rhs);
    }
  };

  template <>
  struct not_equal_to<kagami::Object> {
    bool operator()(kagami::Object const &lhs, kagami::Object const &rhs) const {
      auto copy_lhs = lhs, copy_rhs = rhs;
      return !kagami::management::type::CompareObjects(copy_lhs, copy_rhs);
    }
  };
}

namespace kagami {
  using ObjectTable = unordered_map<Object, Object>;
  using ManagedTable = shared_ptr<ObjectTable>;
}

namespace mgmt = kagami::management;
namespace ext = kagami::management::extension;
namespace rt = kagami::management::runtime;

#define EXPORT_CONSTANT(ID) management::CreateConstantObject(#ID, Object(ID))


