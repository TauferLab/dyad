from spack.package import *

class Dyad(AutotoolsPackage):

    homepage = "https://dyad.readthedocs.io"
    git = "https://github.com/flux-framework/dyad"

    maintainers("JaeseungYeom", "ilumsden")

    version("main", branch="main")
    version("0.1.1", tag="v0.1.1", preferred=True)
    version("0.1.1", tag="v0.1.1")
    version("0.1.0", tag="v0.1.0")

    variant("debug", default="False",
            description="Enable debug logging and prints")
    variant("perfflow", default="False",
            description="Enable performance measurement with PerfFlow Aspect")

    depends_on("flux-core")
    depends_on("jansson@2.10:")
    depends_on("ucx@1.6.0:")
    depends_on("perfflow-aspect", when="+perfflow")

    def autoreconf(self, spec, prefix):
        Executable("./autogen.sh")()

    def configure_args(self):
        args = []
        args += self.enable_or_disable("dyad-debug", variant="debug")
        args += self.enable_or_disable("perfflow")
        return args
