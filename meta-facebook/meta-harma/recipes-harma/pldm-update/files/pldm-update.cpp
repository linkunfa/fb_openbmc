
#include "pldm-update.hpp"

#include <sdbusplus/bus.hpp>
#include <libpldm/utils.h>
#include <libpldm/firmware_update.h>

#include <fcntl.h>    // open
#include <unistd.h>   // open, read
#include <filesystem> // path, copy
#include <thread>     // sleep_for
#include <chrono>     // seconds
#include <sys/file.h>

namespace fs = std::filesystem;

static constexpr auto softwareRoot = "/xyz/openbmc_project/software";

class FileDescriptor {
public:
    explicit FileDescriptor(int fd) : fd(fd) {}
    ~FileDescriptor() { if (fd >= 0) close(fd); }
    operator int() const { return fd; }  // Allow using this class wherever an int is expected.
private:
    int fd;
};

struct PldmUpdateLock
{
  int fd = -1;
  PldmUpdateLock() 
  {
    fd = open("/tmp/pldm-update-ag.lock", O_CREAT | O_RDWR, 0666);
    if (fd < 0)
    {
      std::cerr << "Cannot create/open /tmp/pldm-update-ag.lock" << std::endl;
      throw std::runtime_error("Cannot create!");
    }
    if (flock(fd, LOCK_EX | LOCK_NB) < 0)
    {
      close(fd);
      fd = -1;
      throw std::runtime_error(
        "Update aborted: The same process is still ongoing.");
    }
  }
  ~PldmUpdateLock()
  {
    if (fd < 0)
      return;
    flock(fd, LOCK_UN);
    close(fd);
  }
};

template <typename Func>
bool retry(Func func, int maxRetries, int delayS = 1)
{
    for (int attempt = 1; attempt <= maxRetries; ++attempt)
    {
        if (func())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(delayS));
    }
    return false;
}

bool
is_pldmd_service_running(std::string& _pldmdBusName)
{
    try {
        // use system1 to method "GetUnit"
        auto bus = sdbusplus::bus::new_default();
        auto service = "org.freedesktop.systemd1";
        auto objpath = "/org/freedesktop/systemd1";
        auto target = "pldmd.service";

        // get pldmd.service object path
        // append target to message
        auto msg = bus.new_method_call(
            service,
            objpath,
            "org.freedesktop.systemd1.Manager",
            "GetUnit");
        msg.append(target);
        auto pldmdObjPath = sdbusplus::message::object_path();
        auto reply = bus.call(msg);
        reply.read(pldmdObjPath);

        // get property ActiveState
        // append pldmd.service object path to message and ActiveState
        msg = bus.new_method_call(
            service,
            pldmdObjPath.str.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get"
        );
        msg.append("org.freedesktop.systemd1.Unit", "ActiveState");
        // use variant for dynamic type
        auto activeState = std::variant<std::string>();
        reply = bus.call(msg);
        reply.read(activeState);

        // result
        if (std::get<std::string>(activeState) != "active")
        {
            return false;
        }

        // get Bus Name of pldmd.service
        msg = bus.new_method_call(
            service,
            pldmdObjPath.str.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get"
        );
        msg.append("org.freedesktop.systemd1.Service", "BusName");
        auto pldmdBusName = std::variant<std::string>();
        reply = bus.call(msg);
        reply.read(pldmdBusName);
        _pldmdBusName = std::get<std::string>(pldmdBusName);

        std::cout << "pldmd.service : "
            << _pldmdBusName
            << std::endl;

    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

std::string
get_compute_software_id(const std::string& file)
{
    std::string softwareId{};

    FileDescriptor fd(open(file.c_str(), O_RDONLY));
    if (fd < 0) {
        std::cerr << "Cannot open "
                << file
                << std::endl;
        return softwareId;
    }

    auto offset = sizeof(pldm_package_header_information);
    auto pkgData = std::vector<uint8_t>(offset);
    auto pkgHdr = reinterpret_cast<pldm_package_header_information*>(pkgData.data());

    // read Package Header Info
    if (read(fd, pkgData.data(), offset) != (int)offset) {
        std::cerr << "Can not read "
                << file
                << std::endl;
        return softwareId;
    }
    auto pkgVerLen = pkgHdr->package_version_string_length;
    pkgData.resize(offset + pkgVerLen);
    if (read(fd, pkgData.data() + offset, pkgVerLen) != (int)pkgVerLen) {
        std::cerr << "Can not read "
                << file
                << std::endl;
        return softwareId;
    }
    auto str = std::string(
        reinterpret_cast<const char*>(pkgData.data()+offset), pkgVerLen);

    softwareId = std::to_string(std::hash<std::string>{}(str));
    return softwareId;
}

bool
check_pldmd_software_object(const std::string& pldmdBusName, 
                            const std::string& softwareId)
{
    try {
        auto bus = sdbusplus::bus::new_default();
        auto swObjPath = std::string(softwareRoot) + "/" + softwareId;
        auto interface = "org.freedesktop.DBus.Introspectable";
        auto method = "Introspect";

        auto msg = bus.new_method_call(
            pldmdBusName.c_str(),
            swObjPath.c_str(),
            interface,
            method);
        auto reply = bus.call(msg);

    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool
delete_software_id(const std::string& pldmdBusName, 
                   const std::string& softwareId)
{
    if (!check_pldmd_software_object(pldmdBusName, softwareId))
    {
        // software id object doesn't exist, we assume this operation success
        return true;
    }

    auto bus = sdbusplus::bus::new_default();
    auto swObjPath = std::string(softwareRoot) + "/" + softwareId;

    auto method = bus.new_method_call(pldmdBusName.c_str(), 
                                      swObjPath.c_str(), 
                                      "org.freedesktop.DBus.Properties", 
                                      "Get");
    method.append("xyz.openbmc_project.Software.Activation", "Activation");
    auto activationState = std::variant<std::string>();

    try
    {
        auto reply = bus.call(method);
        reply.read(activationState);

        // if pldm update is activating, cannot call the delete method
        if (std::get<std::string>(activationState) == 
            "xyz.openbmc_project.Software.Activation.Activations.Activating")
        {
            return false;
        }
    }
    catch(const sdbusplus::exception_t& e)
    {
        std::cerr << "Failed to get activation state: " << e.what() << std::endl;
        return false;
    }

    method = bus.new_method_call(pldmdBusName.c_str(), 
                                 swObjPath.c_str(), 
                                 "xyz.openbmc_project.Object.Delete", 
                                 "Delete");
    try
    {
        bus.call_noreply(method);
    }
    catch(const sdbusplus::exception_t& e)
    {
        std::cerr << "Error deleting software ID " << softwareId << ": "
                  << e.what() << std::endl;
        return false;
    }

    return true;
}

void
active_update(const std::string& pldmdBusName, const std::string& softwareId)
{
    try {
        auto bus = sdbusplus::bus::new_default();
        auto swObjPath = std::string(softwareRoot) + "/" + softwareId;
        auto interface = "xyz.openbmc_project.Software.Activation";
        auto property = "RequestedActivation";
        auto value = std::variant<std::string>{
            "xyz.openbmc_project.Software.Activation.RequestedActivations.Active"};

        auto msg = bus.new_method_call(
            pldmdBusName.c_str(),
            swObjPath.c_str(),
            "org.freedesktop.DBus.Properties",
            "Set");

        msg.append(interface, property, value);
        auto reply = bus.call(msg);

    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

uint8_t
get_progress(const std::string& pldmdBusName, const std::string& softwareId)
{
    try {
        auto bus = sdbusplus::bus::new_default();
        auto swObjPath = std::string(softwareRoot) + "/" + softwareId;
        auto interface = "xyz.openbmc_project.Software.ActivationProgress";
        auto property = "Progress";

        auto msg = bus.new_method_call(
            pldmdBusName.c_str(),
            swObjPath.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get");
        msg.append(interface, property);
        auto progress = std::variant<uint8_t>();
        auto reply = bus.call(msg);
        reply.read(progress);

        return std::get<uint8_t>(progress);

    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 0xFF;
    }
}

void
PldmUpdateApp::pldm_update(const std::string& file)
{
    std::unique_ptr<PldmUpdateLock> lock;
    try
    {
        lock = std::make_unique<PldmUpdateLock>();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!fs::exists(file))
    {
        std::cerr << "Error: file " << file << " does not exist." << std::endl;
        return;
    }

    auto softwareId = get_compute_software_id(file);
    if (softwareId.empty())
    {
        std::cerr << "Error: software ID is invalid." << std::endl;
        return;
    }

    std::cout << "pldm_update: " << file << std::endl;
    std::cout << "softwareId: " << softwareId << std::endl;

    try {
        auto src = fs::path(file);
        auto dest = fs::path("/tmp/pldm_images") / src.filename();
        fs::copy(src, dest, fs::copy_options::overwrite_existing);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
    
    // wait for pldmd.service process
    if (!retry([&]() {
        return check_pldmd_software_object(pldmdBusName, softwareId);
    }, 10 /* attempts */, 3 /* delay in seconds */))
    {
        std::cerr << "Time out: pldmd.service did not process image.\n";
        return;
    }

    active_update(pldmdBusName, softwareId);
    for (int time = 0; time <= 200; ++time)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto progress = get_progress(pldmdBusName, softwareId);

        if (progress == 0xFF) {
            break;
        } else if (time == 200) {
            std::cout << "\nTime out : pldmd.service did not finish update."
                      << std::endl;
            break;
        } else if (progress <= 100) {
            std::cout << "\rCurrent progress: "
                      << static_cast<int>(progress) << "/100 (" << time << "s)" 
                      << std::flush;
            if (progress == 100) {
                std::cout << "\nUpdate of software ID " << softwareId 
                          << ": succeeded." << std::endl;
                break;
            }
        }
    }

    if (!retry([&]() { 
        return delete_software_id(pldmdBusName, softwareId);
    }, 10 /* attempts */))
    {
        std::cerr << "Failed to delete software ID: "
                  << "Object " << softwareRoot << "/" << softwareId
                  << " is stuck in \"Activating\" state." << std::endl;
        return;
    }
}

int
main(int argc, char** argv)
{
    PldmUpdateApp app("\nPLDM update tool. (Work with pldmd.service)\n");

    // check pldmd.service first
    if (!is_pldmd_service_running(app.pldmdBusName)) {
        std::cerr << "pldmd.service is not running." << std::endl;
        return -1;
    }

    app.add_options();
    app.run(argc, argv);
    return 0;
}