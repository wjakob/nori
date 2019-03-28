from __future__ import print_function
import os
import subprocess
import sys

TEST_SCENES = [
    "pa4/tests/test-mesh.xml",
    "pa4/tests/test-mesh-furnace.xml",
    # "pa5/tests/chi2test-microfacet.xml",
    # "pa5/tests/ttest-microfacet.xml",
    # "pa5/tests/test-direct.xml",
    # "pa5/tests/test-furnace.xml",
]

TEST_WARPS = [
    ("square", None),
    ("tent", None),
    ("disk", None),
    ("uniform_sphere", None),
    ("uniform_hemisphere", None),
    ("cosine_hemisphere", None),
    ("beckmann", 0.05),
    ("beckmann", 0.10),
    ("beckmann", 0.30),
    # ("microfacet_brdf", 0.01),
    # ("microfacet_brdf", 0.10),
    # ("microfacet_brdf", 0.30),
]

def find_build_directory():
    root = os.path.join(".")
    if os.path.isfile(os.path.join(root, "nori")):
        return root
    root = os.path.dirname(root)
    if os.path.isfile(os.path.join(root, "nori")):
        return root
    return os.path.join(root, "build")


def test_warps_and_scenes(scenes, warps):
    total = len(scenes) + len(warps)
    passed = 0
    failed = []
    build_dir = find_build_directory()

    for t in scenes:
        path = os.path.join("scenes", t)
        ret = subprocess.call([os.path.join(build_dir, "nori"), path])
        if ret == 0:
            passed += 1
        else:
            failed.append(t)

    for (warp_type, param) in warps:
        args = [os.path.join(build_dir, "warptest"), warp_type]
        if param is not None:
            if not isinstance(param, (list, tuple)):
                param = [param]
            for p in param:
                args.append(str(p))
        ret = subprocess.call(args)
        if ret == 0:
            passed += 1
        else:
            failed.append(' '.join(args))

    print("")
    if passed < total:
        print("\033[91m" + "Passed " + str(passed) + " / " + str(total) + " tests." + "\033[0m")
        print("Failed tests:")
        for t in failed:
            print("\t" + str(t))
        return False
    else:
        print("\033[92m" + "Passed " + str(passed) + " / " + str(total) + " tests." + "\033[0m")
        print("\tGood job!")

    return True



if __name__ == '__main__':
    if not test_warps_and_scenes(TEST_SCENES, TEST_WARPS):
        sys.exit(1)
