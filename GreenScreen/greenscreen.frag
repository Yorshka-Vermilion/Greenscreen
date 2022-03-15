#version 130

uniform sampler2D tex;
uniform sampler2D img;
uniform vec3 low;
uniform vec3 high;
uniform int shader;
uniform vec2 start;
uniform vec2 end;
uniform vec2 resolution;

out vec4 color;

float insideBox(vec2 v, vec2 bottomLeft, vec2 topRight) {
    vec2 s = step(bottomLeft, v) - step(topRight, v);
    return s.x * s.y;   
}

void main()
{
	vec2 position = (gl_FragCoord.xy/resolution.xy);
	vec4 colorTMP = texture2D(tex,position);
	if(shader == 1){
		if(colorTMP.x > low.x && colorTMP.y > low.y && colorTMP.z > low.z && colorTMP.x < high.x && colorTMP.y < high.y && colorTMP.z < high.z){
			if(start.x == end.x && start.y == end.y){
				color = texture2D(img,position);
			}
			else if(insideBox(gl_FragCoord.xy, start, end)==1) {
				color = texture2D(img,position);
			}else color = colorTMP;
		}else color = colorTMP;
	}else color = colorTMP;
}