from spack.package import *

class PerfflowAspect(CMakePackage):

    homepage = "https://perfflowaspect.readthedocs.io"
    git = "https://github.com/flux-framework/PerfFlowAspect"

    version("main", branch="main")

    depends_on("libllvm@6.0:")
    depends_on("jannson@2.6:")
    depends_on("openssl@1.0.2d:")
    depends_on("flex@2.5.37:")
    depends_on("bison@3.0.4:")

    requires("%clang@6.0:",
             msg="PerfFlow Aspect is a C/C++ Clang plugin, so it requires Clang")
