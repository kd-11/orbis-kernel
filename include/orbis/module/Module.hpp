#pragma once

#include "ModuleHandle.hpp"
#include "ModuleSegment.hpp"

#include "../utils/Rc.hpp"

#include "orbis-config.hpp"
#include <cstddef>
#include <vector>
#include <string>

namespace orbis {
struct Thread;
struct Process;

struct ModuleNeeded {
  std::string name;
  std::uint16_t version;
  std::uint16_t attr;
};

enum class SymbolBind : std::uint8_t {
  Local,
  Global,
  Weak,
  Unique = 10
};

enum class SymbolVisibility : std::uint8_t {
  Default,
  Internal,
  Hidden,
  Protected
};

enum class SymbolType : std::uint8_t {
  NoType,
  Object,
  Function,
  Section,
  File,
  Common,
  Tls,
  IFunc = 10,
};


struct Symbol {
  std::int32_t moduleIndex;
  std::uint32_t libraryIndex;
  std::uint64_t id;
  std::uint64_t address;
  std::uint64_t size;
  SymbolVisibility visibility;
  SymbolBind bind;
  SymbolType type;
};

struct Relocation {
  std::uint64_t offset;
  std::uint32_t relType;
  std::uint32_t symbolIndex;
  std::int64_t addend;
};

struct Module {
  Process *proc{};
  std::string vfsPath;
  char name[256]{};
  ModuleHandle id{};
  uint32_t tlsIndex{};
  ptr<void> tlsInit{};
  uint32_t tlsInitSize{};
  uint32_t tlsSize{};
  uint32_t tlsOffset{};
  uint32_t tlsAlign{};
  ptr<void> initProc{};
  ptr<void> finiProc{};
  ptr<void> ehFrameHdr{};
  ptr<void> ehFrame{};
  uint32_t ehFrameHdrSize{};
  uint32_t ehFrameSize{};
  ModuleSegment segments[4]{};
  uint32_t segmentCount{};
  std::uint8_t fingerprint[20]{};
  ptr<void> base{};
  uint64_t size{};
  ptr<void> stackStart{};
  ptr<void> stackEnd{};
  ptr<void> processParam{};
  uint64_t processParamSize{};
  ptr<void> moduleParam{};
  uint64_t moduleParamSize{};

  ptr<uint64_t> pltGot{};

  uint16_t version{};
  uint16_t attributes{};
  uint16_t type{};
  uint16_t flags{};
  uint64_t entryPoint{};

  uint32_t phNum{};
  uint64_t phdrAddress{};

  bool isTlsDone = false;

  std::vector<Symbol> symbols;
  std::vector<Relocation> pltRelocations;
  std::vector<Relocation> nonPltRelocations;
  std::vector<ModuleNeeded> neededModules;
  std::vector<ModuleNeeded> neededLibraries;
  std::vector<utils::Ref<Module>> importedModules;

  std::atomic<unsigned> references{0};

  void incRef() {
    if (references.fetch_add(1, std::memory_order::relaxed) > 512) {
      assert(!"too many references");
    }
  }

  void decRef() {
    if (references.fetch_sub(1, std::memory_order::relaxed) == 1 && proc != nullptr) {
      destroy();
    }
  }

  orbis::SysResult relocate(Process *process);

private:
  void destroy();
};

utils::Ref<Module> createModule(Thread *p, std::string vfsPath, const char *name);
} // namespace orbis
