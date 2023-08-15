#version 330

#define MAX_STEPS 100
#define MAX_DIST 100.
#define SURF_DIST .01

precision mediump float;

in vec2 texcoords;

uniform int window_width;
uniform int window_height;

out vec4 out_Color;

uniform float iTime;                 // shader playback time (in seconds)

float rangeMod(float x, float start, float end) {
	float l = end - start;
	return mod(mod(x - start, l) + l, l) + start;
}

vec3 rangeMod(vec3 x, vec3 start, vec3 end) {
	vec3 l = end - start;
	return mod(mod(x - start, l) + l, l) + start;
}

vec3 rangeModWithOffset(vec3 position, vec3 offset, vec3 radius) {
	return rangeMod(position, offset - radius, offset + radius);
}

vec3 rangeModWithOffset(vec3 position, vec3 offset, float radius) {
	return rangeModWithOffset(position, offset, vec3(radius, radius, radius));
}

float sdBox(vec3 p, vec3 b) {
	vec3 q = abs(p) - b;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdCross(vec3 p, float r) {
	r = r / 3.;

	float centerCube = sdBox(p, vec3(r, r, r));
	float leftCube = sdBox(p, vec3(r, r, r) + vec3(0, 0, 2 * r));
	float rightCube = sdBox(p, vec3(r, r, r) + vec3(0, 0, -2 * r));
	float topCube = sdBox(p, vec3(r, r, r) + vec3(0, 2 * r, 0));
	float bottomCube = sdBox(p, vec3(r, r, r) + vec3(0, -2 * r, 0));
	float frontCube = sdBox(p, vec3(r, r, r) + vec3(2 * r, 0, 0));
	float backCube = sdBox(p, vec3(r, r, r) + vec3(-2 * r, 0, 0));

	return min(min(min(min(min(min(centerCube, leftCube), rightCube), topCube), bottomCube), frontCube), backCube);
}

mat3 rot(float roll, float pitch, float yaw) {
	float cr = cos(roll);
	float sr = sin(roll);
	float cp = cos(pitch);
	float sp = sin(pitch);
	float cy = cos(yaw);
	float sy = sin(yaw);

	return mat3(cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr, sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr, -sp, cp * sr, cp * cr);
}

// rotate around a given point
mat4 rot(float roll, float pitch, float yaw, vec3 point) {
	mat3 r = rot(roll, pitch, yaw);
	vec3 p = point - r * point;
	return mat4(r[0], 0, r[1], 0, r[2], 0, p, 1);
}

// Other forms: https://iquilezles.org/articles/distfunctions/
float GetDist(vec3 p) {
	// vec4 s = vec4(0, 1, sin(iTime) * 6, 1);

	float planeDist = p.y;
	float d = MAX_DIST;

	vec3 mengerPos = vec3(0, 10, 20);
	float mengerSize = 5.;
	float ms = mengerSize;
	float mengerCube = sdBox(p - mengerPos, vec3(ms - 0.05, ms - 0.05, ms - 0.05));

	d = mengerCube;

	int itSteps = 5;

	for(int i = 0; i < itSteps; i++) {
		float size = ms / pow(3., float(i));
		vec3 coor = rangeModWithOffset(p, mengerPos, vec3(size, size, size));
		// rotate around the center of the menger cube
		coor = (rot(iTime * 0.5, iTime * 0.5, iTime * 0.5, mengerPos) * vec4(coor, 1)).xyz;
		float crossDist = sdCross(coor - mengerPos, size);
		d = max(-crossDist, d);
	}

	// sphereDist = length(rangeModWithOffset(p,s.xyz, vec3(4, 4, 5)) - s.xyz) - s.w;
	d = max(-p.z + 5., d);

	d = min(d, planeDist);

	float backplane = p.z - 100.;
	d = max(d, backplane);

	// draw everying after the z=2 plane
	d = max(d, -p.z + 2.);
	return d;
}

float RayMarch(vec3 ro, vec3 rd) {
	float dO = 0.;

	for(int i = 0; i < MAX_STEPS; i++) {
		vec3 p = ro + rd * dO;
		float dS = GetDist(p);
		dO += dS;
		if(dO > MAX_DIST || dS < SURF_DIST)
			break;
	}

	return dO;
}

vec3 GetNormal(vec3 p) {
	float d = GetDist(p);
	vec2 e = vec2(.01, 0);

	vec3 n = d - vec3(GetDist(p - e.xyy), GetDist(p - e.yxy), GetDist(p - e.yyx));

	return normalize(n);
}

float GetLight(vec3 p) {
	vec3 lightPos = vec3(0, 5, 6);
	lightPos.xz += vec2(sin(iTime), cos(iTime)) * 2.;
	// lightPos.xz = vec2(sin(2), cos(2)) * 2.;
	vec3 l = normalize(lightPos - p);
	vec3 n = GetNormal(p);
	float dif = clamp(dot(n, l), 0., 1.);

	float d = RayMarch(p + n * SURF_DIST * 2., l);
	if(d < length(lightPos - p))
		dif *= .1;

	return dif;
}

void main() {
	vec2 uv = (texcoords * vec2(window_width, window_height) - vec2(window_width, window_height) * .5) / window_height;

	vec3 col = vec3(0);

	// vec3 ro = vec3(sin(iTime * 1) * 12.5, (cos(iTime*2.)+0.5) * 10 + 20.5, 0);
	vec3 ro = vec3(0, (cos(iTime * 0.5) + 0.5) * 10 + 10, 0);
	vec3 rd = normalize(( rot(v2f_viewRotation.x, v2f_viewRotation.y, v2f_viewRotation.z, ro) * vec4(uv.x, uv.y, 0.5, 0)).xyz);

	float d = RayMarch(ro, rd);

	vec3 p = ro + rd * d;

	float dif = GetLight(p);
	col = vec3(dif);

	col = pow(col, vec3(.4545));	// gamma correction

	gl_FragDepth = d / MAX_DIST;
	// gl_FragColor = vec4(col, 1.0);
	out_Color = vec4(col, 1.0);
}
