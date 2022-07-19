import os

from conans import ConanFile


os.environ["CONAN_CMAKE_GENERATOR"] = "Ninja"


class NNCCConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "entt/3.10.1",
        "glfw/3.3.7",
        "folly/2022.01.31.00",
        "imgui/cci.20220621+1.88.docking",
        "mpdecimal/2.5.1",
        "openssl/1.1.1o",
        "zlib/1.2.12"
    ]
    generators = "CMakeToolchain", "CMakeDeps", "cmake"
    default_options = {"folly:shared": False}
