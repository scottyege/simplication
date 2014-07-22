
attribute vec3 vObjPos;


uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;


void main(void) 
{
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(vObjPos, 1.0f);
}