#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform vec4 a_colour;

void main()
{
	gl_FragColor = a_colour;
}

