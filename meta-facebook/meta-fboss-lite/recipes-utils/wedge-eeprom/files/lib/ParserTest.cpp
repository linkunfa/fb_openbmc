#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdlib.h>
#include <unistd.h>
#include <FbossEepromParser.h>

using EepromData = std::vector<uint8_t>;
using EepromContents = std::vector<std::pair<std::string, std::string>>;

// Based on the Spec for V5 EEPROM:
// https://github.com/facebook/fboss/blob/main/fboss/docs/meta_eeprom_format_v5.md
EepromData eepromV5 = {
    0xfb, 0xfb, 0x05, 0xff, 0x01, 0x0d, 0x46, 0x49, 0x52, 0x53, 0x54, 0x5f,
    0x53, 0x51, 0x55, 0x45, 0x45, 0x5a, 0x45, 0x02, 0x08, 0x32, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x03, 0x08, 0x53, 0x59, 0x53, 0x41, 0x31,
    0x32, 0x33, 0x34, 0x04, 0x0c, 0x50, 0x43, 0x42, 0x41, 0x31, 0x32, 0x33,
    0x34, 0x35, 0x36, 0x37, 0x20, 0x05, 0x0c, 0x50, 0x43, 0x42, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x20, 0x06, 0x0c, 0x4d, 0x59, 0x4f,
    0x44, 0x4d, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x07, 0x0d, 0x4f,
    0x53, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42,
    0x08, 0x01, 0x01, 0x09, 0x01, 0x00, 0x0a, 0x01, 0x01, 0x0b, 0x0d, 0x50,
    0x53, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x41,
    0x0c, 0x07, 0x55, 0x4e, 0x41, 0x5f, 0x4d, 0x41, 0x53, 0x0d, 0x08, 0x32,
    0x30, 0x31, 0x33, 0x30, 0x32, 0x30, 0x33, 0x0e, 0x05, 0x54, 0x45, 0x52,
    0x5a, 0x4f, 0x0f, 0x09, 0x4a, 0x55, 0x49, 0x43, 0x45, 0x54, 0x4f, 0x52,
    0x59, 0x10, 0x07, 0x42, 0x55, 0x44, 0x4f, 0x4b, 0x41, 0x4e, 0x11, 0x08,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x01, 0x02, 0x12, 0x08, 0x12, 0x34,
    0x56, 0x78, 0x9a, 0xbc, 0x03, 0x04, 0x13, 0x08, 0x66, 0x55, 0x44, 0x33,
    0x22, 0x11, 0x02, 0x00, 0x14, 0x08, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54,
    0x00, 0x02, 0xfa, 0x02, 0xd5, 0xc6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

EepromContents expectedContentsV5 = {
    {"Product Name", "FIRST_SQUEEZE"},
    {"Product Part Number", "20123456"},
    {"System Assembly Part Number", "SYSA1234"},
    {"Meta PCBA Part Number", "PCBA1234567 "},
    {"Meta PCB Part Number", "PCB12345678 "},
    {"ODM/JDM PCBA Part Number", "MYODM1234567"},
    {"ODM/JDM PCBA Serial Number", "OS123456789AB"},
    {"Product Production State", "1"},
    {"Product Version", "0"},
    {"Product Sub-Version", "1"},
    {"Product Serial Number", "PS1234567890A"},
    {"System Manufacturer", "UNA_MAS"},
    {"System Manufacturing Date", "20130203"},
    {"PCB Manufacturer", "TERZO"},
    {"Assembled At", "JUICETORY"},
    {"EEPROM location on Fabric", "BUDOKAN"},
    {"X86 CPU MAC Base", "11:22:33:44:55:66"},
    {"X86 CPU MAC Address Size", "258"},
    {"BMC MAC Base", "12:34:56:78:9a:bc"},
    {"BMC MAC Address Size", "772"},
    {"Switch ASIC MAC Base", "66:55:44:33:22:11"},
    {"Switch ASIC MAC Address Size", "512"},
    {"META Reserved MAC Base", "fe:dc:ba:98:76:54"},
    {"META Reserved MAC Address Size", "2"},
    {"CRC16", "0xd5c6"},
};

struct {
  const char* name;
  const EepromData& raw;
  const EepromContents& parsed;
} tests[] = {
  {"V5", eepromV5, expectedContentsV5},
};


void printParsed(const EepromContents& parsed, std::ostream& out) {
  for (const auto& pair : parsed) {
    out << pair.first << " = " << pair.second << std::endl;
  }
}

bool testOne(const EepromData& raw, const EepromContents& expected) {
  char filename[] = "/tmp/parsertest.XXXXXX";
  int fd = mkstemp(filename);
  if (fd == -1) {
    perror("Can't create temp file");
    return false;
  }

  if (write(fd, &raw[0], raw.size()) != (ssize_t)raw.size()) {
    perror("Bad write");
    return false;
  }
  close(fd);

  facebook::fboss::platform::FbossEepromParser parser(filename, 0);
  EepromContents actual = parser.getContents();
  
  if (actual.size() != expected.size()) {
    std::cerr << "Bad parsed size " << actual.size() << ", expected " << expected.size() << std::endl;
    printParsed(actual, std::cerr);
    return false;
  }
  for (size_t i = 0; i < expected.size(); i++) {
    if (expected[i].first != actual[i].first || expected[i].second != actual[i].second) {
      std::cerr << "Difference in row " << i << ". Expected:\n"
        << "  " << expected[i].first << "=" << expected[i].second << "\nGot:\n"
        << "  " << actual[i].first << "=" << actual[i].second << std::endl;
      return false;
    }
  }
  unlink(filename);

  return true;
}

bool testAll() {
  bool success = true;
  for (const auto& test : tests) {
    bool res = testOne(test.raw, test.parsed);
    if (!res) {
      success = false;
      std::cout << test.name << " FAILED!" << std::endl;
    }
  }
  if (success) {
    std::cout << "All tests passed!" << std::endl;
  }
  return success;
}

int main() {
  bool success = testAll();
  return success ? 0 : 1;
}
