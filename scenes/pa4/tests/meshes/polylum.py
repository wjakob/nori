#!python

import numpy as np
import sys

# Make up test cases with polygonal luminaires.

# Step 1: generate a random triangle that lies in the +y half-space.
# Make sure it faces the origin.
v = np.transpose(np.random.rand(3,3) - [[0.5], [0], [0.5]])
normal = np.cross(v[1] - v[0], v[2] - v[0])
if (np.dot(normal, v[0]) > 0):
    v = np.flipud(v)

# Step 2: compute the irradiance using Lambert's formula.
# See Arvo's thesis, equations 3.1 to 3.3.

def norm(x):
    return np.sqrt(np.dot(x,x))

Phi = 0 # vector irradiance
for k0 in range(3):
    k1 = (k0 + 1) % 3
    Theta = np.arccos(np.dot(v[k0], v[k1]) / (norm(v[k0]) * norm(v[k1])))
    Gamma1 = np.cross(v[k0], v[k1])
    Gamma = Gamma1 / norm(Gamma1)
    Phi += 1 / 4.0 * Theta * Gamma

irradiance = -np.dot(Phi, [0,1,0])

# Step 3: write out a nori test scene, wrapped in a t-test

xml_text = """<?xml version="1.0" encoding="utf-8"?>

<test type="ttest">
	<string name="references" value="%g"/>

	<scene>
		<integrator type="path"/>

		<camera type="perspective">
		        <transform name="toWorld">
			        <lookat origin="0, 0.01, 0"
					target="0, 0, 0"
					up="0, 0, 1"/>
			</transform>
			<float name="fov" value="1e-6"/>
			<integer name="width" value="1"/>
			<integer name="height" value="1"/>
		</camera>

		<mesh type="obj">
			<string name="filename" value="floor.obj"/>
			<bsdf type="diffuse">
				<color name="albedo" value="0.5, 0.5, 0.5"/>
			</bsdf>
		</mesh>

		<mesh type="obj">
			<string name="filename" value="%s"/>
			<bsdf type="diffuse">
				<color name="albedo" value="0, 0, 0"/>
			</bsdf>
			<luminaire type="area">
				<color name="radiance" value="1, 1, 1"/>
			</luminaire>
		</mesh>
	</scene>
</test>
"""

obj_text = """v %g %g %g
v %g %g %g
v %g %g %g
f 1 2 3
"""

if len(sys.argv) < 3:
    print "Usage: python polylum.py <xml output file> <obj output file>"
    sys.exit(-1)

fname_obj = sys.argv[2]

f_xml = open(sys.argv[1], 'w')
f_xml.write(xml_text % (.5 / np.pi * irradiance, fname_obj))
f_xml.close()

f_obj = open(fname_obj, 'w')
f_obj.write(obj_text % tuple(v.flat))
f_obj.close()
