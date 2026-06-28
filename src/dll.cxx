#include <iostream>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include "uclib.h"
#include "shm.hpp"
#include "INIReader.h"
#include "utils.hpp"
#include <future>
#include <thread>
#include <nes/named_mutex.hpp>

nng_socket sock;
std::string ipcUrl="";
bool ifDouble = false;
bool precisionChecked = false;


void client(const char *router, const std::string &shm_name, size_t shm_size)
{
    if(ipcUrl != ""){
        nng_req0_open(&sock);
        // Timeout after 1 minute of inactivity to avoid permanent blocking
        nng_socket_set_ms(sock, NNG_OPT_SENDTIMEO, 60000);
        nng_socket_set_ms(sock, NNG_OPT_RECVTIMEO, 60000);
        nng_dial(sock, ipcUrl.c_str(), NULL, 0);
        ipcUrl = "";
    }
    nng_msg *msg;
    nng_msg_alloc(&msg, strlen(shm_name.c_str()) + strlen(router) + 2 + sizeof(size_t));
    strcpy((char *)nng_msg_body(msg), router);
    strcpy((char *)nng_msg_body(msg) + strlen(router) + 1, shm_name.c_str());
    memcpy((char *)nng_msg_body(msg) + strlen(router) + strlen(shm_name.c_str()) + 2, &shm_size, sizeof(size_t));
    int rv = nng_sendmsg(sock, msg, 0);
    if (rv != 0) {
        // Send timeout or failure: free message and return, do not block the caller
        nng_msg_free(msg);
        std::cerr << "nng_sendmsg failed: " << nng_strerror(rv) << std::endl;
        return;
    }
    rv = nng_recvmsg(sock, &msg, 0);
    if (rv != 0) {
        // No response received within 1 minute, timeout
        std::cerr << "nng_recvmsg timeout/failed: " << nng_strerror(rv) << std::endl;
        return;
    }
    nng_msg_free(msg);
    // nng_close(sock);
}

class ShmWriter
{
public:
    ShmWriter(size_t size) : size(size) {}
    template <typename T>
    void write(T *arr, size_t count)
    {
        data.emplace_back(reinterpret_cast<void *>(arr), getBytes(arr, count));
        return;
    }
    void pullResult(const char *router, void *result, u_int resultSize)
    {
        std::string shm_name = generate_unique_shm_name();
        size_t shm_size = sizeof(size_t);
        for (auto &d : data)
        {
            shm_size += d.second;
        }
        shm_size = std::max(sizeof(float)*resultSize, shm_size);
        SharedMemory shm = SharedMemory(shm_name, shm_size, true);
        shm.writeSize(size);
        for (auto &d : data)
        {
            shm.write(d.first, d.second);
        }
        client(router, shm_name, shm_size);
        shm.resetOffset();
        if(ifDouble){
            float *floatResult = static_cast<float *>(shm.data());
            double *doubleResult = static_cast<double *>(result);
            for (size_t i = 0; i < resultSize; i++)
            {
                doubleResult[i] = static_cast<double>(floatResult[i]);
            }
        }else{
            memcpy(result, shm.data(), sizeof(float)*resultSize);
        }
        shm.unlink();
    }


private:
    size_t size;
    std::vector<std::pair<void *, size_t>> data;
};

// Push SCL history data
void USERFUNCTION_EXPORT pushSCLHistory(void* resultIn, int size, CoordReal *meshId, CoordReal *time, CoordReal (*eta)[3], CoordReal (*phi)[3])
{
    if (size == 0)
        return;
    ShmWriter shm = ShmWriter(size);
    shm.write(time, 1);
    shm.write(meshId, size);
    shm.write(eta, size);
    shm.write(phi, size);
    shm.pullResult("pushSCLHistory", resultIn, size);
}
// Push SCL mesh position data
void USERFUNCTION_EXPORT pushSCLPosition(void *resultIn, int size, CoordReal *meshId, CoordReal *time, CoordReal (*coord)[3])
{
    if (size == 0)
        return;
    ShmWriter shm = ShmWriter(size);
    shm.write(time, 1);
    shm.write(meshId, size);
    shm.write(coord, size);
    shm.pullResult("pushSCLPosition", resultIn, size);
}

// Pull SCL potential flow data, components 1-3 are the xyz coordinates of wave surface
void USERFUNCTION_EXPORT getEta(void *resultIn, int size, int *meshId, CoordReal *time)
{
    if (size == 0)
        return;
    ShmWriter shm = ShmWriter(size);
    shm.write(time, 1);
    shm.write(meshId, size);
    shm.pullResult("getEta", resultIn, size*3);
}
// Pull SCL potential flow data, components 1-3 are potential phi, pure wave eta, pure wave phi
void USERFUNCTION_EXPORT getPhi(void *resultIn, int size, int *meshId, CoordReal *time)
{
    if (size == 0)
        return;
    ShmWriter shm = ShmWriter(size);
    shm.write(time, 1);
    shm.write(meshId, size);
    shm.pullResult("getPhi", resultIn, size*3);
}

// Provide VOF wave height data to viscous flow from potential flow
void USERFUNCTION_EXPORT getEtaVOF(void *resultIn, int size, CoordReal (*centroid)[3], CoordReal *time)
{
    if (size == 0)
        return;
    ShmWriter shm = ShmWriter(size);
    shm.write(time, 1);
    shm.write(centroid, size);
    shm.pullResult("getEtaVOF", resultIn, size);
}
// Provide VOF velocity data to viscous flow from potential flow
void USERFUNCTION_EXPORT getVelVOF(void *resultIn, int size, CoordReal (*centroid)[3], CoordReal *time)
{
    if (size == 0)
        return;
    ShmWriter shm = ShmWriter(size);
    shm.write(time, 1);
    shm.write(centroid, size);
    shm.pullResult("getVelVOF", resultIn, size*3);
}

void USERFUNCTION_EXPORT precisionPush(void *resultIn, int size)
{
    auto result = static_cast<float *>(resultIn);
    for (int i = 0; i < size; i++)
    {
        result[i] = 3.0f;
    }
}

void USERFUNCTION_EXPORT precisionPull(void *resultIn, int size, CoordReal *prePush)
{
    if(precisionChecked) return;
    // Check the maximum value of prePush
    double maxVal = 0.0;
    for (int i = 0; i < size; i++)
    {
        if (std::abs(prePush[i]) > maxVal)
        {
            maxVal = std::abs(prePush[i]);
        }
    }
    if(maxVal > 4.0f){
        ifDouble = true;
        std::cout << "Auto detected need for double precision "<< std::endl;
    }else{
        ifDouble = false;
        std::cout << "Auto detected need for single precision "<< std::endl;
    }
    precisionChecked = true;
}

void USERFUNCTION_EXPORT etaVertex(void *resultIn, int size, CoordReal (*eta)[3], CoordReal (*pos)[3])
{
    if (size == 0)
        return;
    ShmWriter shm = ShmWriter(size);
    shm.write(eta, size);
    shm.write(pos, size);
    shm.pullResult("etaVertex", resultIn, size);
}


void initialize()
{
    std::string dllPath = getDllDir();
    INIReader reader(dllPath + "config.ini");
    std::string ipc = reader.Get("SCL", "ipcChannel", "tcp://localhost:4567");
    ipcUrl = ipc;
}

void USERFUNCTION_EXPORT
uclib()
{
    initialize();
    // Viscous flow custom VOF wave
    ucfunc(reinterpret_cast<void *>(getEtaVOF), "ScalarFieldFunction", "etaVOF");
    ucarg(reinterpret_cast<void *>(getEtaVOF), "Vertex", "$$Position", sizeof(CoordReal[3]));
    ucarg(reinterpret_cast<void *>(getEtaVOF), "Vertex", "$Time", sizeof(CoordReal));
    ucarg(reinterpret_cast<void *>(getEtaVOF), "Vertex", "$pushDataReport", sizeof(CoordReal));
    ucfunc(reinterpret_cast<void *>(getVelVOF), "VectorFieldFunction", "velVOF");
    ucarg(reinterpret_cast<void *>(getVelVOF), "Vertex", "$$Position", sizeof(CoordReal[3]));
    ucarg(reinterpret_cast<void *>(getVelVOF), "Vertex", "$Time", sizeof(CoordReal));
    ucarg(reinterpret_cast<void *>(getVelVOF), "Vertex", "$pushDataReport", sizeof(CoordReal));

    // Potential flow data storage
    ucfunc(reinterpret_cast<void *>(getEta), "VectorFieldFunction", "eta");
    ucarg(reinterpret_cast<void *>(getEta), "Cell", "ProstarCellIndex", sizeof(int));
    ucarg(reinterpret_cast<void *>(getEta), "Cell", "$Time", sizeof(CoordReal));
    ucfunc(reinterpret_cast<void *>(getPhi), "VectorFieldFunction", "phi");
    ucarg(reinterpret_cast<void *>(getPhi), "Cell", "ProstarCellIndex", sizeof(int));
    ucarg(reinterpret_cast<void *>(getPhi), "Cell", "$Time", sizeof(CoordReal));

    // Push potential flow data
    ucfunc(reinterpret_cast<void *>(pushSCLHistory), "ScalarFieldFunction", "pushSCLHistory");
    ucarg(reinterpret_cast<void *>(pushSCLHistory), "Cell", "$ProstarCellIndex", sizeof(CoordReal));
    ucarg(reinterpret_cast<void *>(pushSCLHistory), "Cell", "$Time", sizeof(CoordReal));
    ucarg(reinterpret_cast<void *>(pushSCLHistory), "Cell", "$$HistoryofUseretaSample0", sizeof(CoordReal[3]));
    ucarg(reinterpret_cast<void *>(pushSCLHistory), "Cell", "$$HistoryofUserphiSample0", sizeof(CoordReal[3]));
    ucarg(reinterpret_cast<void *>(pushSCLHistory), "Cell", "$UserpushSCLPosition", sizeof(CoordReal));
    ucfunc(reinterpret_cast<void *>(pushSCLPosition), "ScalarFieldFunction", "pushSCLPosition");
    ucarg(reinterpret_cast<void *>(pushSCLPosition), "Cell", "$ProstarCellIndex", sizeof(CoordReal));
    ucarg(reinterpret_cast<void *>(pushSCLPosition), "Cell", "$Time", sizeof(CoordReal));
    ucarg(reinterpret_cast<void *>(pushSCLPosition), "Cell", "$$Centroid", sizeof(CoordReal[3]));
    ucarg(reinterpret_cast<void *>(pushSCLPosition), "Cell", "$UserprecisionPull", sizeof(CoordReal));

    // Automatic single/double precision compatibility
    ucfunc(reinterpret_cast<void *>(precisionPush), "ScalarFieldFunction", "precisionPush");
    ucfunc(reinterpret_cast<void *>(precisionPull), "ScalarFieldFunction", "precisionPull");
    ucarg(reinterpret_cast<void *>(precisionPull), "Cell", "$UserprecisionPush", sizeof(CoordReal));

    // SCL wave surface visualization
    ucfunc(reinterpret_cast<void *>(etaVertex), "ScalarFieldFunction", "etaVertex");
    ucarg(reinterpret_cast<void *>(etaVertex), "Vertex", "$$Usereta", sizeof(CoordReal[3]));
    ucarg(reinterpret_cast<void *>(etaVertex), "Vertex", "$$Position", sizeof(CoordReal[3]));
}