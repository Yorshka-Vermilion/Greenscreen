#version 130

uniform sampler2D tex;
uniform sampler2D img;
uniform vec3 low;
uniform vec3 high;
uniform int shader;

out vec4 color;

void main()
{
	vec2 position = (gl_FragCoord.xy/ (vec2(640,480)));
	vec4 colorTMP = texture2D(tex,position);
	if(shader == 1){
		if(colorTMP.x > low.x && colorTMP.y > low.y && colorTMP.z > low.z && colorTMP.x < high.x && colorTMP.y < high.y && colorTMP.z < high.z)color = texture2D(img,position);
		else color = colorTMP;
	}else color = colorTMP;
	

}