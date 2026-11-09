// JNI bridge: OpenVPN3 ClientAPI <-> Android VpnService.
// OpenVPN3 hands us tun settings through tun_builder_* callbacks; we serialize them to a small
// text spec and let Kotlin build the VpnService.Builder (keeps JNI minimal).
#include <jni.h>
#include <android/log.h>
#include <sstream>
#include <thread>
#include <client/ovpncli.hpp>

using namespace openvpn;
static JavaVM* g_vm = nullptr;
static jobject g_svc = nullptr;

struct Env { JNIEnv* e; bool att = false;
    Env() { if (g_vm->GetEnv((void**)&e, JNI_VERSION_1_6) != JNI_OK) { g_vm->AttachCurrentThread(&e, nullptr); att = true; } }
    ~Env() { if (att) g_vm->DetachCurrentThread(); } };

class Client : public ClientAPI::OpenVPNClient {
    std::ostringstream tun;
public:
    // ---- tun builder ----
    bool tun_builder_new() override { tun.str(""); return true; }
    bool tun_builder_set_remote_address(const std::string&, bool) override { return true; }
    bool tun_builder_add_address(const std::string& a, int p, const std::string&, bool, bool) override { tun << "A " << a << " " << p << "\n"; return true; }
    bool tun_builder_reroute_gw(bool v4, bool v6, unsigned int) override { if (v4) tun << "R 0.0.0.0 0\n"; if (v6) tun << "R :: 0\n"; return true; }
    bool tun_builder_add_route(const std::string& a, int p, int, bool) override { tun << "R " << a << " " << p << "\n"; return true; }
    bool tun_builder_add_dns_server(const std::string& a, bool) override { tun << "D " << a << "\n"; return true; }
    bool tun_builder_set_mtu(int m) override { tun << "M " << m << "\n"; return true; }
    bool tun_builder_set_session_name(const std::string&) override { return true; }
    int tun_builder_establish() override {
        Env env; jclass c = env.e->GetObjectClass(g_svc);
        jmethodID m = env.e->GetMethodID(c, "establish", "(Ljava/lang/String;)I");
        return env.e->CallIntMethod(g_svc, m, env.e->NewStringUTF(tun.str().c_str()));
    }
    void tun_builder_teardown(bool) override {}
    // ---- socket protect (keep OpenVPN's own socket out of the tunnel) ----
    bool socket_protect(int s, std::string, bool) override {
        Env env; jclass c = env.e->GetObjectClass(g_svc);
        return env.e->CallBooleanMethod(g_svc, env.e->GetMethodID(c, "protectSock", "(I)Z"), s);
    }
    // ---- events / logs ----
    void event(const ClientAPI::Event& ev) override {
        Env env; jclass c = env.e->GetObjectClass(g_svc);
        env.e->CallVoidMethod(g_svc, env.e->GetMethodID(c, "onNativeEvent", "(Ljava/lang/String;Ljava/lang/String;)V"),
            env.e->NewStringUTF(ev.name.c_str()), env.e->NewStringUTF(ev.info.c_str()));
    }
    void log(const ClientAPI::LogInfo& l) override { __android_log_print(ANDROID_LOG_INFO, "ovpn3", "%s", l.text.c_str()); }
    void external_pki_cert_request(ClientAPI::ExternalPKICertRequest& r) override { r.error = true; }
    void external_pki_sign_request(ClientAPI::ExternalPKISignRequest& r) override { r.error = true; }
    bool pause_on_connection_timeout() override { return false; }
};

static Client* g_client = nullptr;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) { g_vm = vm; return JNI_VERSION_1_6; }

extern "C" JNIEXPORT void JNICALL
Java_com_example_ovpnapp_OvpnVpnService_nativeStart(JNIEnv* env, jobject thiz, jstring cfg, jstring user, jstring pass) {
    g_svc = env->NewGlobalRef(thiz);
    std::string config = env->GetStringUTFChars(cfg, nullptr), u = env->GetStringUTFChars(user, nullptr), p = env->GetStringUTFChars(pass, nullptr);
    std::thread([config, u, p] {
        g_client = new Client();
        ClientAPI::Config c; c.content = config; c.googleDnsFallback = false;
        ClientAPI::EvalConfig ev = g_client->eval_config(c);
        if (ev.error) { __android_log_print(ANDROID_LOG_ERROR, "ovpn3", "config error: %s", ev.message.c_str()); return; }
        if (!u.empty()) { ClientAPI::ProvideCreds pc; pc.username = u; pc.password = p; g_client->provide_creds(pc); }
        ClientAPI::Status st = g_client->connect();          // blocks until disconnect
        if (st.error) __android_log_print(ANDROID_LOG_ERROR, "ovpn3", "connect: %s %s", st.status.c_str(), st.message.c_str());
        delete g_client; g_client = nullptr;
    }).detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_ovpnapp_OvpnVpnService_nativeStop(JNIEnv*, jobject) { if (g_client) g_client->stop(); }
