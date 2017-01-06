// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Edits made by the Cameron Hardy
// Actually I pretty much re-wrote this, I'm pretty great
// Date:    2016
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
in vec2 uv;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

uniform sampler2D image;		//Syntax for accessing texture
uniform int cState; 				// color filter state
uniform int eState; 				// edge filter state
uniform int bState; 				// blur filter state
uniform float width; 				// window width
uniform float height; 			// window height
uniform float edge_thres;		// 0.2
uniform float edge_thres2;	// 5.0;
uniform int spacebarPressed;

#define HUE_SIZE 6
#define SAT_SIZE 7
#define VAL_SIZE 4
float[HUE_SIZE] hueLevels = float[] (0.0, 140.0, 160.0, 200.0, 240.0, 360.0);
float[SAT_SIZE] satLevels = float[] (0.0, 0.15, 0.30, 0.45, 0.60, 0.80, 1.0);
float[VAL_SIZE] valLevels = float[] (0.0, 0.3, 0.6, 1.0);


// time to convert some RGB values to HSV!
vec3 RGBtoHSV(float r, float g, float b) {
	float minv, maxv, delta;
	vec3 res;

	minv = min(min(r, g), b);
	maxv = max(max(r, g), b);
	res.z = maxv;									// v for value

	delta = maxv - minv;

	if (maxv != 0.0) {
		res.y = delta / maxv;				// s for saturation
	}
	else {
		// R = G = B = 0
		res.y = 0.0;
		return res;
	}

	if (r == maxv) {
		res.x = (g - b) / delta;					// something between yellow and magenta
	}
	else if (g == maxv) {
		res.x = 2.0 + (b - r) / delta;		// something between cyan and yellow
	}
	else {
		res.x = 4.0 + (r - g) / delta; 		// something between magenta and cyan
	}

	res.x = res.x * 60.0;	// use degrees cuz the colors go in a circle
	if (res.x < 0.0) {
		res.x += 360.0;
	}
	return res;
}

vec3 HSVtoRGB(float h, float s, float v) {
	int i;
	float f, p, q, t;
	vec3 res;

	if (s == 0.0) {
		// achromatic (that means greyish)
		res.x = v;
		res.y = v;
		res.z = v;
		return res;
	}

	h /= 60.0;
	i = int(floor(h));
	f = h - float(i);
	p = v * (1.0 - s);
	q = v * (1.0 - s * f);
	t = v * (1.0 - s * (1.0 - f));

	if (i == 0) {
		res = vec3(v, t, p);
	}
	else if (i == 1) {
		res = vec3(q, v, p);
	}
	else if (i == 2) {
		res = vec3(p, v, t);
	}
	else if (i == 3) {
		res = vec3(p, q, v);
	}
	else if (i == 4) {
		res = vec3(t, p, v);
	}
	else {
		res = vec3(v, p, q);
	}
	return res;
}

float nearestLevel(float col, int mode) {
	int levCount;
	if (mode == 0) {
		levCount = HUE_SIZE;
	}
	if (mode == 1) {
		levCount = SAT_SIZE;
	}
	if (mode == 2) {
		levCount = VAL_SIZE;
	}

	for (int i = 0; i < levCount - 1; i++) {
		if (mode == 0) {
			if (col >= hueLevels[i] && col <= hueLevels[i + 1]) {
				return hueLevels[i + 1];
			}
		}
		if (mode == 1) {
			if (col >= satLevels[i] && col <= satLevels[i + 1]) {
				return satLevels[i + 1];
			}
		}
		if (mode == 2) {
			if (col >= valLevels[i] && col <= valLevels[i + 1]) {
				return valLevels[i + 1];
			}
		}
	}
}

// average the pixel intensity from the 3 color channels
float avg_intensity(vec4 pix) {
	return (pix.r + pix.g + pix.b) / 3;
}

vec4 get_pixel(vec2 coords, float x, float y) {
	return texture(image, coords + vec2(x, y));
}

// this crazy wacky function returns the pixel color
float isEdge(vec2 uv) {
	float dxtex = 1.0 / float(textureSize(image, 0));
	float dytex = 1.0 / float(textureSize(image, 0));
	float pix[9];
	int k = -1;
	float delta;

	// read the neighboring pixel intensities, just for funsies
	for (int i = -1; i < 2; i++) {
		for (int j = -1; j < 2; j++) {
			k++;
			pix[k] = avg_intensity(get_pixel(uv, float(i) * dxtex, float(j) * dytex));
		}
	}

	// average the color differences around neighboring pixels
	delta = (	abs(pix[1] - pix[7]) +
						abs(pix[5] - pix[3]) +
						abs(pix[0] - pix[8]) +
						abs(pix[2] - pix[6])) / 4.0;

	return clamp(edge_thres2 * delta, 0.0, 1.0);
}

// just a quick 2d gaussian edge filter function
// calculates the correct coefficient for position [x][y] of the convolution matrix
float gBlur(int x, int y, float o) {
	return (1/(2*3.141592653589793238462643383*o*o))*pow(2.718281828459045235360287471, -((x*x)+(y*y))/(2*o*o));
}

void main(void)
{
	vec4 color;
	float lumlum;		// luminance
	mat3 vSobel;
	mat3 hSobel;
	mat3 uSharp;

	// INCOMING KERNEL MATRICES
	// Vertical Sobel
	vSobel[0] = vec3(-1,  0,  1);
	vSobel[1] = vec3(-2,  0,  2);
	vSobel[2] = vec3(-1,  0,  1);

	// Horizontal Sobel
	hSobel[0] = vec3(-1, -2, -1);
	hSobel[1] = vec3( 0,  0,  0);
	hSobel[2] = vec3( 1,  2,  1);

	// Unsharp Mask
	uSharp[0] = vec3( 0, -1,  0);
	uSharp[1] = vec3(-1,  5, -1);
	uSharp[2] = vec3( 0, -1,  0);

	// rev up those fryers!
	color = texture(image, uv);

	if (eState == 1) {
		// do literally nothing bahaha it's 2AM
	}
	if (eState == 2) {
		color = vec4(0.0);
		for (int j = 0; j < 3; j++) {
			for (int i = 0; i < 3; i++) {
				color += texture(image, (uv + vec2((i-1)/width, (j-1)/height))) * vSobel[i][j];
			}
		}
		color = abs(color); // stay positive ok
	}
	if (eState == 3) {
		color = vec4(0.0);
		for (int j = 0; j < 3; j++) {
			for (int i = 0; i < 3; i++) {
				color += texture(image, (uv + vec2((i-1)/width, (j-1)/height))) * hSobel[i][j];
			}
		}
		color = abs(color); // keep all ur values positive
	}
	if (eState == 4) {
		color = vec4(0.0);
		for (int j = 0; j < 3; j++) {
			for (int i = 0; i < 3; i++) {
				color += texture(image, (uv + vec2((i-1)/width, (j-1)/height))) * uSharp[i][j];
			}
		}
		color = abs(color); // don't be negative!
	}
	if (eState == 5) {
		color = vec4(1.0, 0.0, 0.0, 1.0);

		vec3 colorOrg = texture(image, uv).rgb;
		vec3 vHSV = RGBtoHSV(colorOrg.r, colorOrg.g, colorOrg.b);
		if (spacebarPressed == 1) {
			vHSV.x = nearestLevel(vHSV.x, 0);
			vHSV.y = nearestLevel(vHSV.y, 1);
			vHSV.z = nearestLevel(vHSV.z, 2);
		}
		float edg = isEdge(uv);
		vec3 vRGB = (edg >= edge_thres) ? vec3(0.0) : HSVtoRGB(vHSV.x, vHSV.y, vHSV.z);
		color = vec4(vRGB.x, vRGB.y, vRGB.z, 1);
	}

	// here comes the blur
	if (bState > 0) {
		color = vec4(0.0);
		int n = (2*bState)+1; // odd matrices only pls
		float o = n/5; // make a reasonable value for sigma in the gaussian formula
		for (int j = 0; j < n; j++) {
			for (int i = 0; i < n; i++) {
				color += texture(image, (uv + vec2((i-bState)/width, (j-bState)/height))) * gBlur(i-bState, j-bState, o);
			}
		}
	}

	if (cState == 1) { // normal color
		// don't do a single thing you big silly
	}
	if (cState == 2) { // b&w variant 1
		lumlum = (color.x * 0.333) + (color.y * 0.333) + (color.z * 0.333);
		color.x = lumlum;
		color.y = lumlum;
		color.z = lumlum;
	}
	if (cState == 3) { // b&w variant 2
		lumlum = (color.x * 0.299) + (color.y * 0.587) + (color.z * 0.114); // you're the best around nothins gonna ever keep you down
		color.x = lumlum;
		color.y = lumlum;
		color.z = lumlum;
	}
	if (cState == 4) { // b&w variant 3
		lumlum = (color.x * 0.213) + (color.y * 0.715) + (color.z * 0.072);
		color.x = lumlum;
		color.y = lumlum;
		color.z = lumlum;
	}
	if (cState == 5) { // inverted colors
		color.x = 1-color.x;
		color.y = 1-color.y;
		color.z = 1-color.z;
	}
	if (cState == 6) { // monochromatic reduced color palette
		lumlum = (color.x * 0.299) + (color.y * 0.587) + (color.z * 0.114);

		// set thresholds for the reduced color palette
		if (lumlum > 0.95)
			color = vec4(1.0, 0.5, 0.5, 1.0);
		else if (lumlum > 0.5)
			color = vec4(0.6, 0.3, 0.3, 1.0);
		else if (lumlum > 0.25)
			color = vec4(0.4, 0.2, 0.2, 1.0);
		else
			color = vec4(0.2, 0.1, 0.1, 1.0);
	}
	if (cState == 7) { // full saturation
		// each rgb value gets rounded to either 0.0 or 1.0
		color.x = floor(color.x + 0.5);
		color.y = floor(color.y + 0.5);
		color.z = floor(color.z + 0.5);
	}

	FragmentColour = color;
}
