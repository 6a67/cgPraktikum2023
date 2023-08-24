#version 330

#define MAX_STEPS 200
#define MAX_DIST 150.
#define SURF_DIST .001

#define MOD_TIME mod(iTime, 80.)

precision highp float;

in vec2 texcoords;

uniform int window_width;
uniform int window_height;

uniform float distOffset;

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

mat3 rot_rpy(float roll, float pitch, float yaw) {
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
mat4 rot_rpy(float roll, float pitch, float yaw, vec3 point) {
	mat3 r = rot_rpy(roll, pitch, yaw);
	vec3 p = point - r * point;
	return mat4(r[0], 0, r[1], 0, r[2], 0, p, 1);
}

float sdSphere(vec3 p, float s) {
	return length(p) - s;
}

float sdApollonian(vec3 p, float size, int iterations) {
    // Moving the scene itself forward, as opposed to the camera.
    // IQ does it in one of his small examples.
    // p.z += iTime;

    // Loop counter and variables.
	float s = size;
	float k;


	// Repeat Apollonian distance field. It's just a few fractal related 
    // operations. Break up space, distort it, repeat, etc. More iterations
    // would be nicer, but this function is called a hundred times, so I've
    // used the minimum to give just enough intricate detail.
	for(int i = 0; i < iterations; i++) {
		p *= k = 1.5 / dot(p = mod(p - 1., 2.) - 1., p);
		s *= k;
	}

	// Render numerous little spheres, spread out to fill in the 
    // repeat Apollonian lattice-like structure you see.
    //
    // Note the ".01" at the end. Most people make do without it, but
    // I like the tiny spheres to have a touch more volume, especially
    // when using low iterations.
	return length(p) / s - .01;
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

	vec3 apoPos = vec3(5, 5, 5);

	// p = rangeModWithOffset(p, mengerPos, 5.);

	// if(mod(iTime, 20.) > 10) {
	// 	p.y = rangeMod(p.y, 5, 15);
	// 	p.x = rangeMod(p.x, -5, 5);
	// }

	float apoDist = sdApollonian(p - apoPos, 6., 6);

	// d = min(d, sdSphere(p - vec3(0,10,13), 2)/2);

	d = min(d, apoDist);

	// sphereDist = length(rangeModWithOffset(p,s.xyz, vec3(4, 4, 5)) - s.xyz) - s.w;
	// d = max(-p.z + 5., d);

	d = min(d, planeDist);

	// float backplane = p.z - 100.;
	// d = max(d, backplane);

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
	vec2 e = vec2(.0001, 0);

	vec3 n = d - vec3(GetDist(p - e.xyy), GetDist(p - e.yxy), GetDist(p - e.yyx));

	return normalize(n);
}

vec3 GetLight(vec3 p) {
	if(length(p) >= MAX_DIST - MAX_DIST * 0.3) {
		return vec3(0, 1, 1);
	}

	// phong lighting
	vec3 lightPos = vec3(0, 100, -20);
	lightPos.xz += vec2(sin(iTime), cos(iTime)) * 2.;
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
	float objColRed = clamp(sin(p.x * 2 / MAX_DIST), 0., 1.);
	float objColGreen = clamp(cos(p.y / MAX_DIST), 0., 1.);
	float objColBlue = clamp(sin(p.z / MAX_DIST), 0., 1.);
	vec3 objColor = vec3(objColRed, objColGreen, objColBlue);

	// float d = RayMarch(p + n * SURF_DIST * 2., l);
	// if(d < length(lightPos - p))
	// 	result *= .1;

	// objColor = vec3(1.0);

	// ambient occlusion
	int steps = 5;
	float stepSize = 0.2;	// increase this to increase effect of ambient occlusion
	float sumO = 0.0;
	float maxSum = .0;
	for(int i = 0; i < steps; i++) {
		vec3 pos = p + n * stepSize * float(i + 1);
		float d = GetDist(pos);
		sumO += 1. / pow(2., float(i)) * d;
		maxSum += 1. / pow(2., float(i)) * stepSize * float(i + 1);
	}

	sumO = sumO / maxSum;

	sumO = clamp(sumO, 0., 1.);

	result = result * objColor * res * sumO;
	// boost the brightness
	// result = pow(result, vec3(1));

	return result;
}

vec3 cameraMovement(in vec3[3] points, float start, float end, bool smoothp, in vec3 viewIn, out vec3 viewOut, out mat3 rot) {
	// vec3 points[4] = vec3[](vec3(0, 10, 0), vec3(0, 10, 10), vec3(-30, -10, -5), vec3(0, 10, 20));
	// vec3 points[2] = vec3[](vec3(10, 10, 0), vec3(-10, 10, 0));
	// vec3 points[2] = vec3[](vec3(0, 10, 0), vec3(0, -10, 0));
	// vec3 points[2] = vec3[](vec3(0, 10, -10), vec3(0, 10, 20));

	float diff = end - start;
	float t = (MOD_TIME - start) / diff;
	if(smoothp) {
		t = smoothstep(0., 1., t);
	}

	if(MOD_TIME > end) {
		t = 1;
	}

	if(MOD_TIME < start) {
		t = 0;
	}

	// float t = 1 - (cos(iTime * speed) * 0.5 + 0.5);

	vec3 p = points[0];

	for(int i = 0; i < points.length() - 1; i++) {
		p = mix(p, points[i + 1], t);
	}

	vec3 tangent = normalize(points[1] - points[0]);

	for(int i = 0; i < points.length() - 2; i++) {
		tangent = mix(tangent, normalize(points[i + 2] - points[i + 1]), t);
	}

	// tangent = vec3(0,0,1);

	// rotate viewIn to match tangent
	vec3 up = vec3(0, 1, 0);
	vec3 right = normalize(cross(tangent, up));
	up = normalize(cross(right, tangent));

	rot = mat3(-right, up, tangent);

	viewOut = rot * viewIn;

	return p;
}

vec3 cameraRotate(in vec3 center, in vec3 viewIn) {
	// rotate camera around center
	float alpha = iTime * 3;
	viewIn = (rot_rpy(0, alpha, 0, center) * vec4(viewIn, 0.)).xyz;
	return viewIn;
}

uniform vec3 viewRotation;
uniform vec3 model_pos;
uniform bool draw_face;
uniform vec3 textureRotation;

void main() {
	vec2 uv = (texcoords * vec2(window_width, window_height) - vec2(window_width, window_height) * .5) / window_height;

	// vec3 ro = vec3(0, 10, (sin(iTime * 0.2)) * 10 + 10);

	vec3 ro = vec3(0, 10, 0);

	vec3 rd;
	if(draw_face) {
	  rd = normalize(rot_rpy(textureRotation.x, textureRotation.y, textureRotation.z, ro) * vec4(uv.x, uv.y, 0.5, 0.0)).xyz;
	}else{
	  rd = normalize(rot_rpy(viewRotation.x, viewRotation.y, viewRotation.z, ro) * vec4(uv.x, uv.y, 0.5, 0.0)).xyz;
	}

	vec3 viewOut;
	mat3 rot;

	if(MOD_TIME < 10) {
		vec3 points[3] = vec3[](vec3(0, 10, 0), vec3(0, 10, 10), vec3(0, 10, 20));
		ro = cameraMovement(points, 0, 10, true, rd, viewOut, rot);
		// see getDist for popping in more cubes
	}

	if(MOD_TIME < 30 && MOD_TIME >= 10) {
		vec3 points[3] = vec3[](vec3(0, 10, 20), vec3(0, 10, 20.01), vec3(0, 20, 20.01));
		ro = cameraMovement(points, 10, 30, true, rd, viewOut, rot);
	}

	if(MOD_TIME < 40 && MOD_TIME >= 30) {
		vec3 points[3] = vec3[](vec3(0, 20, 20), vec3(0, 20.01, 20), vec3(0, 20.01, 30));
		ro = cameraMovement(points, 30, 40, false, rd, viewOut, rot);
	}

	// follow
	if(MOD_TIME >= 40 && MOD_TIME < 65) {
		vec3 points[3] = vec3[](vec3(0, 20, 30), vec3(0, 20, 40), vec3(0, 20, 50));
		ro = cameraMovement(points, 40, 60, false, rd, viewOut, rot);
	}

	// teleport camera back to origin
	// if(MOD_TIME > 60) {
	// 	vec3 points[3] = vec3[](vec3(0, 10, 0), vec3(0, 10, 0.01), vec3(0, 10, 0.02));
	// 	ro = cameraMovement(points, 60, 80, false, rd, viewOut, rot);
	// }

	// move camera back to origin
	if(MOD_TIME >= 65) {
		vec3 points[3] = vec3[](vec3(0, 10, 50), vec3(0, 10, 0.02), vec3(0, 10, 0.01));
		ro = cameraMovement(points, 65, 75, true, rd, viewOut, rot);
	}

	rd = normalize(viewOut);
	ro = ro + rot * vec3(model_pos.x, model_pos.y, model_pos.z);

	// invert right and view vector to move backwards
	if(MOD_TIME >= 65) {
		rd.x = -rd.x;
		rd.z = -rd.z;
		ro -= rot * vec3(model_pos.x, model_pos.y, model_pos.z);
		ro += rot * vec3(-model_pos.x, model_pos.y, -model_pos.z);
	}

	float d = RayMarch(ro, rd);
	vec3 p = ro + rd * d;
	vec3 light = GetLight(p);
	light = pow(light, vec3(.4545));	// gamma correction

	float near = 0.2;
	float far = 90.;
	float dz = ((1. / d) - ( 1. / near))/((1./far) - (1./near));
	gl_FragDepth = dz;

	// gl_FragColor = vec4(col, 1.0);
	out_Color = vec4(light, 1.0);
	// out_Color = vec4(texcoords,0.,1.);
}
