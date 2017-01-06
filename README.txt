CPSC 453
Assignment 2

Cameron Hardy
ID 10084560

+--Operating Procedures--+
|  Step 1: Open terminal |
|  Step 2: make          |
|  Step 3: ./a.out       |
+------------------------+

Choose image with number keys 1-6
	1: image1-mandrill.png
	2: image2-uclogo.png
	3: image3-aerial.jpg
	4: image4-thirsk.jpg
	5: image5-pattern.png
	6: ohboy.jpg
Choose color effect with keys q, w, e, r, t, y, u
	q: Original color
	w: Black and white when L = (0.333R + 0.333G + 0.333B)
	e: Black and white when L = (0.299R + 0.587G + 0.114B) (this one is the best, it is closest to natural human vision sensitivity, and it gives the green bit a wide gamut (approx. +/- 0.5), which is good cuz we're more sensitive to that color
	r: Black and white when L = (0.213R + 0.715G + 0.072B)
	t: Inverted color
	y: Monochromatic 4-color palette (Gameboy style)
	u: Full saturation
Choose edge effect with keys a, s, d, f
	a: No edge effect
	s: Vertical Sobel
	d: Horizontal Sobel
	f: Unsharp mask
	g: Crazy cel shader - Use ctrl+arrow keys to change line thresholds (Use this filter on image 1, 7, and 8!) Also press m to activate crazy color mode!
Increase/decrease blur with keys z, x
Translate image with arrow keys
Zoom/rotate with shift+arrow keys

Got source code for the crazy cel shader from http://www.geeks3d.com/20140523/glsl-shader-library-toonify-post-processing-filter/, modified and repurposed it to work with my code the way I want. It's pretty dope 

Please note that since blur uses a convolution matrix, it cannot be combined with other edge effects. :(

