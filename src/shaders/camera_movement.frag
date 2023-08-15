#version 330

#define MAX_STEPS 50
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

mat3 rot(float roll, float pitch, float yaw) {
	float cr = cos(roll);
	float sr = sin(roll);
	float cp = cos(pitch);
	float sp = sin(pitch);
	float cy = cos(yaw);
	float sy = sin(yaw);

	return mat3(cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr, sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr, -sp, cp * sr, cp * cr);
}

float sdBox(vec3 p, vec3 b) {
	vec3 q = abs(p) - b;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// rotate around a given point
mat4 rot(float roll, float pitch, float yaw, vec3 point) {
	mat3 r = rot(roll, pitch, yaw);
	vec3 p = point - r * point;
	return mat4(r[0], 0, r[1], 0, r[2], 0, p, 1);
}

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}

float sdMenger(vec3 p, float size, int iterations) {
    //p=rotateY(p,iTime*.5);
	vec3[] s = vec3[](vec3(1, 1, 1), vec3(1, 1, 0));

	for(int iter = 0; iter < iterations; ++iter) {
		// float alpha = iTime * 0.1;
		// p = p * rot(0, 0, alpha);
		// float beta = iTime * 0.15;
		// p = p * rot(0, beta, 0);
		// float gamma = iTime * 0.05;
		// p = p * rot(gamma, 0, 0);

		p = abs(p);
		if(p.y > p.x)
			p.yx = p.xy;
		if(p.z > p.y)
			p.zy = p.yz;

		if(p.z > .5 * size)
			p -= size * s[0];
		else
			p -= size * s[1];
		size /= 3.;

	}
	return sdSphere(p, vec3(size).x);
}

float sdBoxFrame(vec3 p, vec3 b, float e) {
	p = abs(p) - b;
	vec3 q = abs(p + e) - e;
	return min(min(length(max(vec3(p.x, q.y, q.z), 0.0)) + min(max(p.x, max(q.y, q.z)), 0.0), length(max(vec3(q.x, p.y, q.z), 0.0)) + min(max(q.x, max(p.y, q.z)), 0.0)), length(max(vec3(q.x, q.y, p.z), 0.0)) + min(max(q.x, max(q.y, p.z)), 0.0));
}

// Other forms: https://iquilezles.org/articles/distfunctions/
float GetDist(vec3 p) {
	// vec4 s = vec4(0, 1, sin(iTime) * 6, 1);

	float planeDist = p.y;
	float d = MAX_DIST;

	vec3 mengerPos = vec3(0, 10, 20);

	// p = rangeModWithOffset(p, mengerPos, 5.);

	float mengerDist = sdMenger(p - mengerPos, 3., 7);

	d = min(d, mengerDist);

	// sphereDist = length(rangeModWithOffset(p,s.xyz, vec3(4, 4, 5)) - s.xyz) - s.w;
	// d = max(-p.z + 5., d);

	d = min(d, planeDist);

	float backplane = p.z - 100.;
	d = max(d, backplane);

	// draw everying after the z=2 plane
	// d = max(d, -p.z + 2.);
	return d;
}

float RayMarch(vec3 ro, vec3 rd) {
	float dO = 0.;
	int i = 0;

	for(i = 0; i < MAX_STEPS; i++) {
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

vec3 GetLight(vec3 p) {
	if (length(p) >= MAX_DIST - 1) {
		return vec3(0,1,1);
	}

	// phong lighting
	vec3 lightPos = vec3(0, 100, 0);
	// lightPos.xz += vec2(sin(iTime), cos(iTime)) * 2.;
	lightPos.y += sin(iTime * 0.5) * 80.;

	float red = 0.5 + (sin(iTime * 0.1) + 1) * 0.5;
	float green = 0.5 + (cos(iTime * 0.1) + 1) * 0.5;
	float blue = 0.5 + (sin(iTime * iTime * 0.1) + 1) * 0.5;
	// vec3 lightColor = vec3(red, green, blue);
	vec3 lightColor = vec3(1.0);

	vec3 n = GetNormal(p);
	vec3 l = normalize(lightPos - p);
	vec3 v = normalize(vec3(0, 0, 0) - p);
	vec3 r = reflect(-l, n);

	float sum = 0.1 + max(dot(n, l), 0.) + pow(max(dot(r, v), 0.), 16.);
	vec3 result = lightColor * sum;

	// soft shadows
	float res = 1.0;
	float t = 0.0;

	int i = 0;

	for(i = 0; i < 64; i++) {
		float h = GetDist(p + l * t);
		res = min(res, 8 * h / t);
		if(res < 0.001)
			break;
		t += clamp(h, 0.01, 0.2);
	}

	res = clamp(res, 0.1, 1.);

	// object color
	float objColRed = clamp(sin(p.x * 2/ MAX_DIST), 0., 1.);
	float objColGreen = clamp(cos(p.y / MAX_DIST), 0., 1.);
	float objColBlue = clamp(sin(p.z / MAX_DIST), 0., 1.);
	vec3 objColor = vec3(objColRed, objColGreen, objColBlue);

	// float d = RayMarch(p + n * SURF_DIST * 2., l);
	// if(d < length(lightPos - p))
	// 	result *= .1;

	// objColor = vec3(1.0);

	return result * res * objColor;
}


vec3 cameraMovement(in vec3 viewIn, out vec3 viewOut) {
	vec3 points[4] = vec3[](
		vec3(0, 10, 0),
		vec3(0, 10, 10),
		vec3(-20, -7, -10),
		vec3(0, 10, 20)
	);

	float t = sin(iTime * 1) * 0.5 + 0.5;

	vec3 p = points[0];

	for(int i = 0; i < points.length() - 1; i++) {
		p = mix(p, points[i + 1], t);
	}

	vec3 tangent = normalize(points[1] - points[0]);

	for(int i = 0; i < points.length() - 2; i++) {
		tangent = mix(tangent, normalize(points[i + 2] - points[i + 1]), t);
	}

	// rotate viewIn to match tangent
	vec3 up = vec3(0, 1, 0);
	vec3 right = normalize(cross(up, tangent));
	up = normalize(cross(tangent, right));

	mat3 rot = mat3(right, up, tangent);

	viewOut = rot * viewIn;

	return p;
}

vec3 cameraRotate(in vec3 center, in vec3 viewIn) {
	// rotate camera around center
	float alpha = iTime * 3;
	viewIn = (rot(0, alpha, 0, center) * vec4(viewIn, 0.)).xyz;
	return viewIn;
}

void main() {
	vec2 uv = (texcoords * vec2(window_width, window_height) - vec2(window_width, window_height) * .5) / window_height;

	vec3 col = vec3(0);

	// vec3 ro = vec3(0, 10, (sin(iTime * 0.2)) * 10 + 10);

	vec3 ro = vec3(0, 10, 0);
	vec3 rd = normalize(vec3(uv.x, uv.y, 1));
	// vec3 viewOut;
	// vec3 ro = cameraMovement(rd, viewOut);
	// rd = normalize(viewOut);
	// rd = cameraRotate(vec3(0, 10, 20), rd);

	// rd = rot(0, iTime, 0) * rd;

	float d = RayMarch(ro, rd);

	vec3 p = ro + rd * d;

	vec3 light = GetLight(p);

	light = pow(light, vec3(.4545));	// gamma correction

	gl_FragDepth = clamp(d / MAX_DIST, -0.999, 0.999);
	// gl_FragColor = vec4(col, 1.0);
	out_Color = vec4(light, 1.0);
}
