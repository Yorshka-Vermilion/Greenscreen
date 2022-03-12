#version 130

uniform sampler2D tex;
uniform sampler2D img;

out vec4 color;

void main()
{
	vec2 position = (gl_FragCoord.xy/ (vec2(640,480)));
	vec4 colorTMP = texture2D(tex,position);
	if(colorTMP.x > 0.5) color = vec4(0,0,0,1); //texture2D(img,position);
	else color = colorTMP;

}