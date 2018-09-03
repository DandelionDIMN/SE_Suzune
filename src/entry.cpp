#include "entry.h"
#include <iostream>

namespace kagami {
  bool Entry::Compare(Entry &target) const {
    return (target.id == this->id &&
      target.activity == this->activity &&
      target.parmMode == this->parmMode &&
      target.priority == this->priority &&
      this->type == target.type &&
      target.args == this->args);
  }

  Message Entry::Start(ObjectMap &objMap) const {
    if (placeholder) return Message();
    Message result;
    if (userFunc) {
      objMap[kStrUserFunc] = Object()
        .Manage(id)
        .SetMethods(type::GetPlanner(kTypeIdRawString)->GetMethods())
        .SetTokenType(T_GENERIC);
    }
    if (Good()) {
      result = activity(objMap);
    }
    else {
      result.combo(kStrFatalError, kCodeIllegalCall, "Illegal entry.");
    }
    return result;
  }

  namespace entry {
    list<ObjectManager> &GetObjectStack() {
      static list<ObjectManager> base;
      return base;
    }

    vector<Entry> &GetEntryBase() {
      static vector<Entry> base;
      return base;
    }

    map<GenericTokenEnum, Entry> &GetGenProviderBase() {
      static map<GenericTokenEnum, Entry> base;
      return base;
    }

    Object *FindObject(string sign) {
      Object *object = nullptr;
      size_t count = GetObjectStack().size();
      list<ObjectManager> &base = GetObjectStack();

      while (!base.empty() && count > 0) {
        object = base[count - 1].Find(sign);
        if (object != nullptr) {
          break;
        }
        count--;
      }
      return object;
    }

    ObjectManager &GetCurrentManager() {
      return GetObjectStack().back();
    }

    Object *FindObjectInCurrentManager(string sign) {
      Object *object = nullptr;
      ObjectManager &base = GetObjectStack().back();

      while (!base.Empty()) {
        object = base.Find(sign);
        if (object != nullptr) {
          break;
        }
      }

      return object;
    }

    Object *CreateObject(string sign, Object &object) {
      ObjectManager &base = GetObjectStack().back();

      base.Add(sign, object);
      const auto result = base.Find(sign);
      return result;
    }

    string GetTypeId(const string sign) {
      auto result = kTypeIdNull;
      auto count = GetObjectStack().size();
      auto &base = GetObjectStack();

      while (count > 0) {
        const auto object = base[count - 1].Find(sign);
        if (object != nullptr) {
          result = object->GetTypeId();
        }
        count--;
      }

      return result;
    }

    void ResetObject() {
      while (!GetObjectStack().empty()) GetObjectStack().pop_back();
    }

    ObjectManager &CreateManager() {
      auto &base = GetObjectStack();
      base.push_back(std::move(ObjectManager()));
      return GetObjectStack().back();
    }

    bool DisposeManager() {
      auto &base = GetObjectStack();
      if (!base.empty()) { base.pop_back(); }
      return base.empty();
    }

    map<string, GenericTokenEnum> &GetGTBase() {
      using T = pair<string, GenericTokenEnum>;
      static map<string, GenericTokenEnum> base = {
        T(kStrIf,GT_IF),
        T(kStrNop,GT_NOP),
        T(kStrDef,GT_DEF),
        T(kStrRef,GT_REF),
        T(kStrEnd,GT_END),
        T(kStrSet,GT_SET),
        T(kStrBind,GT_BIND),
        T(kStrFor,GT_FOR),
        T(kStrElse,GT_ELSE),
        T(kStrElif,GT_ELIF),
        T(kStrWhile,GT_WHILE),
        T(kStrCodeSub,GT_CODE_SUB),
        T(kStrLeftSelfInc,GT_LSELF_INC),
        T(kStrLeftSelfDec,GT_LSELF_DEC),
        T(kStrRightSelfInc,GT_RSELF_INC),
        T(kStrRightSelfDec,GT_RSELF_DEC),
        T(kStrAdd,GT_ADD),
        T(kStrSub,GT_SUB),
        T(kStrMul,GT_MUL),
        T(kStrDiv,GT_DIV),
        T(kStrIs,GT_IS),
        T(kStrLessOrEqual,GT_LESS_OR_EQUAL),
        T(kStrMoreOrEqual,GT_MORE_OR_EQUAL),
        T(kStrNotEqual,GT_NOT_EQUAL),
        T(kStrMore,GT_MORE),
        T(kStrLess,GT_LESS),
        T(kStrReturn,GT_RETURN)
      };
      return base;
    }

    GenericTokenEnum GetGenericToken(string src) {
      auto &base = GetGTBase();
      auto it = base.find(src);
      if (it != base.end()) return it->second;
      return GT_NUL;
    }

    bool IsOperatorToken(GenericTokenEnum token) {
      return(token == GT_ADD || token == GT_SUB || token == GT_MUL || token == GT_DIV
        || token == GT_IS || token == GT_LESS_OR_EQUAL || token == GT_MORE_OR_EQUAL
        || token == GT_NOT_EQUAL || token == GT_MORE || token == GT_LESS);
    }

    string GetGenTokenValue(GenericTokenEnum token) {
      auto &base = GetGTBase();
      string result = kStrNull;
      for (auto &unit : base) {
        if (unit.second == token) {
          result = unit.first;
          break;
        }
      }
      return result;
    }

    OperatorCode GetOperatorCode(string src) {
      if (src == "+")  return ADD;
      if (src == "-")  return SUB;
      if (src == "*")  return MUL;
      if (src == "/")  return DIV;
      if (src == "=")  return EQUAL;
      if (src == "==") return IS;
      if (src == "<=") return LESS_OR_EQUAL;
      if (src == ">=") return MORE_OR_EQUAL;
      if (src == "!=") return NOT_EQUAL;
      if (src == ">")  return MORE;
      if (src == "<")  return LESS;
      if (src == "++") return SELFINC;
      if (src == "--") return SELFDEC;
      return NUL;
    }

    void AddEntry(Entry temp) {
      GetEntryBase().emplace_back(temp);
    }

    void AddGenericEntry(GenericTokenEnum token, Entry temp) {
      GetGenProviderBase().insert(pair<GenericTokenEnum, Entry>(
        token, temp));
    }

    Entry GetGenericProvider(GenericTokenEnum token) {
      auto &base = GetGenProviderBase();
      map<GenericTokenEnum, Entry>::iterator it = base.find(token);
      if (it != base.end()) return it->second;
      return Entry();
    }


    Entry Order(string id, string type, int size) {
      GenericTokenEnum basicOpCode = GetGenericToken(id);
      if (basicOpCode != GT_NUL) {
        return GetGenericProvider(basicOpCode);
      }

      vector<Entry> &base = GetEntryBase();
      Entry result;
      //TODO:rewrite here
      for (auto &unit : base) {
        if (id == unit.GetId() && type == unit.GetSpecificType()
          && (size == -1 || size == int(unit.GetParmSize()))) {
          result = unit;
          break;
        }
      }
      return result;
    }
  }

  namespace type {
    map <string, ObjectPlanner> &GetPlannerBase() {
      static map<string, ObjectPlanner> base;
      return base;
    }

    shared_ptr<void> GetObjectCopy(Object &object) {
      if (object.ConstructorFlag()) {
        return object.Get();
      }

      shared_ptr<void> result = nullptr;
      const auto option = object.GetTypeId();
      const auto it = GetPlannerBase().find(option);

      if (it != GetPlannerBase().end()) {
        result = it->second.CreateObjectCopy(object.Get());
      }
      return result;
    }

    ObjectPlanner *GetPlanner(const string name) {
      ObjectPlanner *result = nullptr;
      const auto it = GetPlannerBase().find(name);

      if (it != GetPlannerBase().end()) {
        result = &(it->second);
      }
      return result;
    }

    void AddTemplate(string name, ObjectPlanner temp) {
      GetPlannerBase().insert(pair<string, ObjectPlanner>(name, temp));
    }

    void DisposeTemplate(const string name) {
      const auto it = GetPlannerBase().find(name);
      if (it != GetPlannerBase().end()) GetPlannerBase().erase(it);
    }
  }
}