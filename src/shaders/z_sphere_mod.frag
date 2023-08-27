#version 330

#define MAX_STEPS 100
#define MAX_DIST 100.
#define SURF_DIST .01

precision mediump float;

in vec2 texcoords;

uniform int window_width;
uniform int window_height;

// out vec4 f_color;
// in vec2 fragCoord;

out vec4 out_Color;

uniform vec3 viewRotation;

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
	return rangeMod(position, offset - radius, offset+radius);
}

vec3 rangeModWithOffset(vec3 position, vec3 offset, float radius) {
	return rangeModWithOffset(position, offset, vec3(radius, radius , radius));
}

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

// Other forms: https://iquilezles.org/articles/distfunctions/
float GetDist(vec3 p) {
	vec4 s = vec4(0, 0, 6, 1);

	float sphereDist = 0;
	float d = MAX_DIST;

	sphereDist = length(p - s.xyz) - s.w;

	// inf in all directions
	// sphereDist = length(rangeModWithOffset(p,s.xyz, vec3(4, 4, 5)) - s.xyz) - s.w;

	// individual inf
	// p.x = rangeMod(p.x, s.x - 4., s.x + 4.);
	// p.y = rangeMod(p.y, s.y - 4., s.y + 4.);
	// p.z = rangeMod(p.z, s.z - 5., s.z + 5.);
	// sphereDist = length(p - s.xyz) - s.w;

	d = max(-p.z + 5., sphereDist);


	float backplane = p.z - 100.;
	d = max(d, backplane);

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
	if(length(p) >= MAX_DIST - MAX_DIST * 0.3) {
		return 1.;
	}

	vec3 lightPos = vec3(0,0,0);
	// lightPos.xz = vec2(sin(2), cos(2)) * 2.;
	vec3 l = normalize(lightPos - p);
	vec3 n = GetNormal(p);
	float dif = clamp(dot(n, l), 0., 1.);

	// float d = RayMarch(p + n * SURF_DIST * 2., l);
	// if(d < length(lightPos - p))
	// 	dif *= .1;

	return dif;
}

void main() {
	vec2 uv = (texcoords * vec2(window_width, window_height) - vec2(window_width, window_height) * .5) / window_height;

	vec3 col = vec3(0);

	vec3 ro = vec3(0, 0, 0);
	vec3 rd = normalize(vec3(uv.x, uv.y, 1));

	float d = RayMarch(ro, rd);

	vec3 p = ro + rd * d;

	float dif = GetLight(p);
	col = vec3(dif);

	// for not showing warnings
	col += viewRotation * 0.0001; 

	col = pow(col, vec3(.4545));	// gamma correction
	
	gl_FragDepth = 0.5 + iTime * 0.00001;
	out_Color = vec4(col, 1.0);
	//gl_FragColor = vec4(uv.xy,0.0, 1.0);
}
