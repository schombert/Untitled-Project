#include "globalhelpers.h"
#include "mapdisplay.h"
#include "structs.hpp"
#include <gl\GL.h>
#include <gl\GLU.h>
#include <amp_math.h>
#include "prov_control.h"
#include "i18n.h"
#include "wardata.h"
#include "laws.h"
#include "datamanagement.hpp"

GLuint ProgramID = 0;
GLuint flat_ProgramID = 0;

GLint projection_l = 0;
GLint rot_l = 0;
GLint texture_l = 0;

GLint flat_projection_l = 0;
GLint flat_rot_l = 0;


GLuint new_array;

#define USE_SPH

const static double hpi = 1.57079632679489661923132169163975144;
const static double pi = 3.14159265358979323846264338327950288;
const static double pi180 = pi / 180.0;
const static double tex_width = 3800.0*2.0;
const static double tex_height = 2319.0*2.0;


glm::dvec3 texture_to_sphere(IN(glm::dvec2) source) noexcept;

void create_province_program() noexcept {


	std::string sphere_vetrex_shader = \
		"uniform mat4 rotationm;\n"\
		"uniform mat4 projection;\n"\
		"const float hpi = 1.5707963267;\n"\
		"const float mw = 3000.0;\n"\
		"const float mh = 3000.0;\n"\
		"void main() {\n"\
			"vec4 stp1 = rotationm * gl_Vertex;\n"\
			"vec2 stp2 = vec2(atan(stp1.y, stp1.x), hpi - acos(stp1.z));\n"\
			"float rdiv = stp2.y / 3.14159265358;\n"\
			"if(stp2.x > 3 || stp2.x < -3) {\n"\
				"gl_FrontColor = vec4(0,0,0,0);\n"\
			"} else {\n"\
				"gl_FrontColor = gl_Color;}\n"\
			"vec2 stp3 = vec2(mw * stp2.x * sqrt(1.0 - (3.0*rdiv*rdiv)), stp2.y * mh);\n"\
			"gl_Position = projection * vec4(stp3, 0.0, 1.0);\n"\
			"gl_TexCoord[0] = gl_MultiTexCoord0;\n"\
		"}\n";/**/

	std::string flat_sphere_vetrex_shader = \
		"uniform mat4 rotationm;\n"\
		"uniform mat4 projection;\n"\
		"const float hpi = 1.5707963267;\n"\
		"const float mw = 3000.0;\n"\
		"const float mh = 3000.0;\n"\
		"void main() {\n"\
		"vec4 stp1 = rotationm * gl_Vertex;\n"\
		"vec2 stp2 = vec2(atan(stp1.y, stp1.x), hpi - acos(stp1.z));\n"\
		"float rdiv = stp2.y / 3.14159265358;\n"\
		"if(stp2.x > 3 || stp2.x < -3) {\n"\
		"gl_FrontColor = vec4(0,0,0,0);\n"\
		"} else {\n"\
		"gl_FrontColor = gl_Color;}\n"\
		"vec2 stp3 = vec2(mw * stp2.x * sqrt(1.0 - (3.0*rdiv*rdiv)), stp2.y * mh);\n"\
		"gl_Position = projection * vec4(stp3, 0.0, 1.0);\n"\
		"}\n";/**/


	std::string fragment_shader = \
		"uniform sampler2D texture;\n"\
		"void main() {\n"\
		"gl_FragColor = gl_Color * texture2D(texture, gl_TexCoord[0].xy);\n"\
		"}\n";/**/

	std::string flat_fragment_shader = \
		"void main() {\n"\
		"gl_FragColor = gl_Color;"\
		"}\n";/**/

	OutputDebugStringA(sphere_vetrex_shader.c_str());
	OutputDebugStringA(fragment_shader.c_str());

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	OutputDebugStringA("VERTEX SHADER\r\n");
	char const * VertexSourcePointer = sphere_vetrex_shader.c_str();
	GLint sz = static_cast<GLint>(sphere_vetrex_shader.size());

	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, &sz);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	OutputDebugStringA(&VertexShaderErrorMessage[0]);
	OutputDebugStringA("\r\n");

	// Compile Fragment Shader
	OutputDebugStringA("FRAGMENT SHADER\r\n");
	char const * FragmentSourcePointer = fragment_shader.c_str();
	sz = static_cast<GLint>(fragment_shader.size());
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, &sz);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	OutputDebugStringA(&FragmentShaderErrorMessage[0]);
	OutputDebugStringA("\r\n");

	// Link the program
	OutputDebugStringA("Linking program\n");
	ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage(std::max(InfoLogLength, int(1)));
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	OutputDebugStringA(&ProgramErrorMessage[0]);
	OutputDebugStringA("\r\n");

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);
	
	//glUseProgram(ProgramID);

	projection_l = glGetUniformLocation(ProgramID, "projection");
	rot_l = glGetUniformLocation(ProgramID, "rotationm");
	texture_l = glGetUniformLocation(ProgramID, "texture");

	//
	//
	// FLAT SHADERS
	//
	//


	// Create the shaders
	VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	Result = GL_FALSE;

	// Compile Vertex Shader
	OutputDebugStringA("VERTEX SHADER\r\n");
	VertexSourcePointer = flat_sphere_vetrex_shader.c_str();
	sz = static_cast<GLint>(flat_sphere_vetrex_shader.size());

	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, &sz);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	VertexShaderErrorMessage.resize(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	OutputDebugStringA(&VertexShaderErrorMessage[0]);
	OutputDebugStringA("\r\n");

	// Compile Fragment Shader
	OutputDebugStringA("FRAGMENT SHADER\r\n");
	FragmentSourcePointer = flat_fragment_shader.c_str();
	sz = static_cast<GLint>(flat_fragment_shader.size());
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, &sz);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	FragmentShaderErrorMessage.resize(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	OutputDebugStringA(&FragmentShaderErrorMessage[0]);
	OutputDebugStringA("\r\n");

	// Link the program
	OutputDebugStringA("Linking program\n");
	flat_ProgramID = glCreateProgram();
	glAttachShader(flat_ProgramID, VertexShaderID);
	glAttachShader(flat_ProgramID, FragmentShaderID);
	glLinkProgram(flat_ProgramID);

	// Check the program
	glGetProgramiv(flat_ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(flat_ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	ProgramErrorMessage.resize(std::max(InfoLogLength, int(1)));
	glGetProgramInfoLog(flat_ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	OutputDebugStringA(&ProgramErrorMessage[0]);
	OutputDebugStringA("\r\n");

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	//glUseProgram(flat_ProgramID);

	flat_projection_l = glGetUniformLocation(flat_ProgramID, "projection");
	flat_rot_l = glGetUniformLocation(flat_ProgramID, "rotationm");



	glUseProgram(0);


	OutputDebugStringA("DONE\r\n");

}

void fake_load_display_data(IN(flat_multimap<edge, std::vector<sf::Vector2<short>>>) brder) noexcept {
	global::prov_display.resize(detail::provinces.size());
	global::prov_display.shrink_to_fit();

	for (size_t indx = 1; indx < global::prov_display.size(); ++indx) {
		IN(auto) inter = detail::provinces[indx].intersect;
		__int64 total_area = 0;
		__int64 xtotal = 0;
		__int64 ytotal = 0;
		for (size_t i = inter.index.size() - 1; i != SIZE_MAX; --i) {
			const auto pr = inter.get_row(i);
			size_t rowsz = std::distance(pr.first, pr.second);
			for (size_t j = 0; j != rowsz; j += 2) {
				const auto part_sz = inter.get(i, j + 1).first - inter.get(i, j).first;
				total_area += part_sz;
				xtotal += (inter.get(i, j).first + part_sz / 2) * part_sz;
				ytotal += i * part_sz;
			}
		}
		detail::provinces[indx].centroid = texture_to_sphere(glm::dvec2(static_cast<double>(xtotal) / static_cast<double>(total_area), static_cast<double>(detail::provinces[indx].bounds.top) + static_cast<double>(ytotal) / static_cast<double>(total_area)));
	}

	
	global::borders.clear();
	for (IN(auto) bpr : brder) {
		global::borders.emplace(bpr.first, border());
	}
	global::borders.shrink_to_fit();
}

void load_display_data(IN(std::vector<cvector<sf::Vector2f>>) quads, IN(flat_multimap<edge, std::vector<sf::Vector2<short>>>) brder, IN(OGLLock) win) noexcept {
	glUseProgram(ProgramID);

	for (size_t indx = 1; indx < global::prov_display.size(); ++indx) {
		glDeleteBuffers(1, &global::prov_display[indx].vertexbuffer);
	}

	global::prov_display.resize(detail::provinces.size());
	global::prov_display.shrink_to_fit();

	for (size_t indx = 1; indx < global::prov_display.size(); ++indx) {
		glGenBuffers(1, &global::prov_display[indx].vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, global::prov_display[indx].vertexbuffer);
		global::prov_display[indx].numvertex = static_cast<GLsizei>(quads[indx].size());
		GLfloat* buf = new GLfloat[global::prov_display[indx].numvertex * 5];
		size_t off = 0;

		//double totalx = 0.0;
		//double totaly = 0.0;
		//double cnt = 0.0;
		for (const auto &ps : quads[indx]) {
			__analysis_assume(off == 0);
			glm::dvec3 vertex = texture_to_sphere(glm::dvec2(ps.x, ps.y));
			buf[off * 5 + 0] = static_cast<GLfloat>(ps.x / tex_width);
			buf[off * 5 + 1] = static_cast<GLfloat>(ps.y / tex_height);
			buf[off * 5 + 2] = static_cast<GLfloat>(vertex.x);
			buf[off * 5 + 3] = static_cast<GLfloat>(vertex.y);
			buf[off * 5 + 4] = static_cast<GLfloat>(vertex.z);
			++off;

			//totalx += static_cast<double>(ps.x);
			//totaly += static_cast<double>(ps.y);
			//++cnt;
		}
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 5 * global::prov_display[indx].numvertex, buf, GL_STATIC_DRAW);
		delete[] buf;

		IN(auto) inter = detail::provinces[indx].intersect;
		__int64 total_area = 0;
		__int64 xtotal = 0;
		__int64 ytotal = 0;
		for (size_t i = inter.index.size() - 1; i != SIZE_MAX; --i) {
			const auto pr = inter.get_row(i);
			size_t rowsz = std::distance(pr.first, pr.second);
			for (size_t j = 0; j != rowsz; j += 2) {
				const auto part_sz = inter.get(i, j + 1).first - inter.get(i, j).first;
				total_area += part_sz;
				xtotal += (inter.get(i, j).first + part_sz / 2) * part_sz;
				ytotal += i * part_sz;
			}
		}

		detail::provinces[indx].centroid = texture_to_sphere(glm::dvec2(static_cast<double>(xtotal) / static_cast<double>(total_area), static_cast<double>(detail::provinces[indx].bounds.top) + static_cast<double>(ytotal) / static_cast<double>(total_area)));
		//global::provinces[indx].quads.clear();
	}

	for (IN(auto) bpr : global::borders) {
		glDeleteBuffers(1, &bpr.second.vertexbuffer);
	}
	global::borders.clear();

	for (IN(auto) bpr : brder) {
		border temp;
			glGenBuffers(1, &temp.vertexbuffer);
			glBindBuffer(GL_ARRAY_BUFFER, temp.vertexbuffer);
			temp.numvertex = static_cast<GLsizei>(bpr.second.size());
			GLfloat* buf = new GLfloat[temp.numvertex * 3];
			size_t off = 0;
			for (const auto &ps : bpr.second) {
				glm::dvec3 vertex = texture_to_sphere(glm::dvec2(ps.x, ps.y));
				__analysis_assume(off == 0);
				buf[off * 3] = static_cast<GLfloat>(vertex.x);
				buf[off * 3 + 1] = static_cast<GLfloat>(vertex.y);
				buf[off * 3 + 2] = static_cast<GLfloat>(vertex.z);
				++off;
			}
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * temp.numvertex, buf, GL_STATIC_DRAW);
			delete[] buf;

			global::borders.emplace(bpr.first, temp);
		
	}
	global::borders.shrink_to_fit();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}


glm::dvec4 apply_matrix(IN(glm::dmat4) m, IN(glm::dvec3) v)  noexcept {
	return m * glm::dvec4(v, 1.0);
}

const static glm::dvec3 haxis(0.0, 1.0, 0.0);
const static glm::dvec3 vaxis(0.0, 0.0, 1.0);

glm::dmat4 generate_mat_transform(const double horz_rotation, const double vert_rotation) noexcept {
	//return glm::rotate(glm::rotate(glm::mat4(1.0f), horz_rotation, glm::dvec3(0.0f, 0.0f, 1.0f)), vert_rotation, glm::dvec3(0.0f,1.0f,0.0f));
	return glm::rotate(glm::rotate(glm::dmat4(1.0), vert_rotation, haxis), horz_rotation, vaxis);
}

glm::dmat4 generate_mat_inverse_transform(const double horz_rotation, const double vert_rotation) noexcept {
	//return glm::rotate(glm::rotate(glm::mat4(1.0f), -vert_rotation, glm::dvec3(0.0f, 1.0f, 0.0f)), -horz_rotation, glm::dvec3(0.0f, 0.0f, 1.0f));
	return glm::rotate(glm::rotate(glm::dmat4(1.0), -horz_rotation, vaxis), -vert_rotation, haxis);
}

glm::dvec3 sphere_from_globe(IN(glm::dvec2) source) noexcept {
	const auto cosy = fast_math::cos(source.y);
	return glm::dvec3(fast_math::cos(source.x)*cosy, fast_math::sin(source.x)*cosy, fast_math::sin(source.y));
}



glm::dvec2 globe_from_sphere(IN(glm::dvec3) source) noexcept {
	return glm::dvec2(fast_math::atan2(source.y, source.x), hpi - fast_math::acos(source.z));
}

const static double map_width = 3000.0;
const static double map_height = 3000.0;

glm::dvec2 map_from_globe(IN(glm::dvec2) source) noexcept {
	const double rdiv = source.y / static_cast<float>(pi);
	return glm::dvec2(map_width * source.x * fast_math::sqrt(1.0 - (3.0*rdiv*rdiv)), source.y * map_height);
}

glm::dvec2 globe_from_map(IN(glm::dvec2) source) noexcept {
	const double yres = source.y / map_height;
	const double rdiv = yres / pi;
	return glm::dvec2(source.x / (map_width * fast_math::sqrt(1.0 - (3.0*rdiv*rdiv))), yres);
}


const static double top_lat = -72.208;
const static double bottom_lat = 0.727;
const static double left_lon = -24.665;
const static double right_lon = 96.261;/**/

glm::dvec2 globe_from_texture(IN(glm::dvec2) source) noexcept {
	return glm::dvec2(pi180 * (left_lon + source.x * (right_lon - left_lon) / tex_width), pi180 * (top_lat + source.y * ( bottom_lat - top_lat) / tex_height));
}

glm::dvec2 texture_from_globe(IN(glm::dvec2) source) noexcept {
	return glm::dvec2(((source.x / pi180) - left_lon) * tex_width / (right_lon - left_lon), ((source.y / pi180) - top_lat) * tex_height / (bottom_lat - top_lat));
}

glm::dvec3 texture_to_sphere(IN(glm::dvec2) source) noexcept {
	return sphere_from_globe(globe_from_texture(source));
}

const double distance_factor = 1000.0;
// const half_fp hp_distance_factor = half_fp::from_double(distance_factor);

double distance_from_globe(IN(glm::dvec2) a, IN(glm::dvec2) b) noexcept {
	return distance_factor * fast_math::acos(fast_math::cos(b.y)*fast_math::cos(a.y)*fast_math::cos(a.x -b.x) + fast_math::sin(a.y)*fast_math::sin(b.y));
}

double distance_from_texture(IN(glm::dvec2) a, IN(glm::dvec2) b) noexcept {
	return distance_from_globe(globe_from_texture(a), globe_from_texture(b));
}

double distance_from_map(IN(glm::dvec2) a, IN(glm::dvec2) b) noexcept {
	return distance_from_globe(globe_from_map(a), globe_from_map(b));
}

double province_distance(IN(province) a, IN(province) b) noexcept {
	return distance_factor * acos(glm::dot(a.centroid, b.centroid));
}

/*
half_fp hp_distance_from_globe(IN(glm::tvec2<half_fp>) a, IN(glm::tvec2<half_fp>) b) noexcept {
	half_fp cosay;
	half_fp sinay;
	half_fp cosby;
	half_fp sinby;
	half_fp cosxidf;
	half_fp sinydif;
	a.y.sincos(sinay, cosay);
	b.y.sincos(sinby, cosby);
	(a.x - b.x).sincos(sinydif, cosxidf);
	sinay *= sinby;
	((cosby *= cosay) *= cosxidf) += sinay;
	cosby.s_acos();
	cosby *= hp_distance_factor;
	return cosby;
	//return hp_distance_factor * (cosby*cosay*cosxidf + sinay*sinby).acos();
}

const static half_fp hp_top_lat = half_fp::from_double(top_lat);
const static half_fp hp_bottom_lat = half_fp::from_double(bottom_lat);
const static half_fp hp_left_lon = half_fp::from_double(left_lon);
const static half_fp hp_right_lon = half_fp::from_double(right_lon);
const static half_fp hp_pi180 = half_fp::from_double(pi180);
const static half_fp hp_pi = half_fp::from_double(pi);
const static half_fp hp_tex_width = half_fp::from_double(tex_width);
const static half_fp hp_tex_height = half_fp::from_double(tex_height);
const static half_fp hp_map_width = half_fp::from_double(map_width);
const static half_fp hp_map_height = half_fp::from_double(map_height);


glm::tvec2<half_fp> hp_globe_from_texture(IN(glm::tvec2<half_fp>) source) noexcept {
	static const half_fp long_dif = hp_right_lon - hp_left_lon;
	static const half_fp lat_dif = hp_bottom_lat - hp_top_lat;
	half_fp x(source.x);
	x *= long_dif;
	x += hp_left_lon;
	x *= hp_pi180;
	x /= hp_tex_width;
	half_fp y(source.y);
	y *= lat_dif;
	y += hp_top_lat;
	y *= hp_pi180;
	y /= hp_tex_height;
	return glm::tvec2<half_fp>(x, y);
}

glm::tvec2<half_fp> hp_globe_from_map(IN(glm::tvec2<half_fp>) source) noexcept {
	const half_fp yres = source.y / hp_map_height;
	const half_fp rdiv = yres / hp_pi;
	return glm::tvec2<half_fp>(source.x / (hp_map_width * sqrt(half_fp::from_int(1) - (half_fp::from_int(3)*rdiv*rdiv))), yres);
}

half_fp hp_distance_from_texture(IN(glm::tvec2<half_fp>) a, IN(glm::tvec2<half_fp>) b) noexcept {
	return hp_distance_from_globe(hp_globe_from_texture(a), hp_globe_from_texture(b));
}

half_fp hp_distance_from_map(IN(glm::tvec2<half_fp>) a, IN(glm::tvec2<half_fp>) b) noexcept {
	return hp_distance_from_globe(hp_globe_from_map(a), hp_globe_from_map(b));
}
*/

glm::dvec2 texture_to_map(IN(glm::dvec2) source, IN(glm::dmat4) rotation) noexcept {
	const glm::dvec4 im = rotation * glm::dvec4(sphere_from_globe(globe_from_texture(source)),1.0);
	return map_from_globe(globe_from_sphere(glm::dvec3(im.x, im.y, im.z)));
}

glm::dvec2 texture_to_globe(IN(glm::dvec2) source, IN(glm::dmat4) rotation) noexcept {
	const glm::dvec4 im = rotation * glm::dvec4(sphere_from_globe(globe_from_texture(source)), 1.0);
	return globe_from_sphere(glm::dvec3(im.x, im.y, im.z));
}

glm::dvec2 map_to_texture(IN(glm::dvec2) source, IN(glm::dmat4) inverse_rotation) noexcept {
	const glm::dvec4 im = inverse_rotation * glm::dvec4(sphere_from_globe(globe_from_map(source)),1.0);
	return texture_from_globe(globe_from_sphere(glm::dvec3(im.x, im.y, im.z)));
}

glm::dvec3 map_to_sphere(IN(glm::dvec2) source, IN(glm::dmat4) inverse_rotation) noexcept {
	const glm::dvec4 im = inverse_rotation * glm::dvec4(sphere_from_globe(globe_from_map(source)), 1.0);
	return glm::dvec3(im.x, im.y, im.z);
}

glm::dvec2 map_to_globe(IN(glm::dvec2) source, IN(glm::dmat4) inverse_rotation) noexcept {
	const glm::dvec4 im = inverse_rotation * glm::dvec4(sphere_from_globe(globe_from_map(source)), 1.0);
	return globe_from_sphere(glm::dvec3(im.x, im.y, im.z));
}

glm::dvec2 sphere_to_map(IN(glm::dvec3) source, IN(glm::dmat4) rotation) noexcept {
	const glm::dvec4 im = rotation * glm::dvec4(source, 1.0);
	return map_from_globe(globe_from_sphere(glm::dvec3(im.x, im.y, im.z)));
}

glm::dvec2 sphere_to_globe(IN(glm::dvec3) source, IN(glm::dmat4) rotation) noexcept {
	const glm::dvec4 im = rotation * glm::dvec4(source, 1.0);
	return globe_from_sphere(glm::dvec3(im.x, im.y, im.z));
}

glm::dvec2 sphere_to_texture(IN(glm::dvec3) source) noexcept {
	return texture_from_globe(globe_from_sphere(source));
}

double i_normalize(double i) noexcept {
	while (i > pi * 2.0)
		i -= pi * 2.0;
	while (i < -pi * 2.0)
		i += pi * 2.0;
	return i;
}

glm::dvec2 nr_step(glm::dvec2 current_step, glm::dvec3 source, glm::dvec3 target) noexcept {
	const double cos_x = cos(current_step.x);
	const double sin_x = sin(current_step.x);
	const double cos_y = cos(current_step.y);
	const double sin_y = sin(current_step.y);

	double jd = -cos_x * sin_y * source.x + sin_x * sin_y * source.y + cos_y * source.z;
	double jc = -sin_x * cos_y * source.x - cos_y * cos_x * source.y;
	
	//double jb = -cos_y * cos_x * source.x + cos_y * sin_x * source.y - sin_y * source.z;
	//double ja = sin_y * sin_x * source.x + sin_y * cos_x * source.y;
	double jb = 0.0;
	double ja = cos_x*source.x - sin_x*source.y;

	double gy = -(cos_y * cos_x * source.x - cos_y * sin_x * source.y + sin_y * source.z - target.x);
	//double gx = -(sin_y * cos_x * source.x + sin_y * sin_x * source.y + cos_y * source.z - target.z);
	double gx = -(sin_x * source.x + cos_x * source.y - target.y);

	jb /= ja;
	gx /= ja;
	//ja = 1.0;

	jd -= jb * jc;
	gy -= gx * jc;
	//jc = 0.0;

	gy /= jd;
	//jd = 1.0;

	gx -= jb * gy;
	//jb = 0.0;

	return glm::dvec2(current_step.x + gx, current_step.y + gy);
}

std::pair<double, double> solve_trig_polynominal(double a, double b, double c, INOUT(bool) nosolution) noexcept { //a * sin(x) + b * cos(x) + c = 0

	const double inner = sqrt(a*a + b*b);
	if (abs(c) > abs(inner)) {
		nosolution = true;
		return std::pair<double, double>(0.0, 0.0);
	}
	const double at = atan2(b, a);
	const double as = asin(-c / inner);

	return std::pair<double, double>(as - at, -as + pi - at);
}

glm::dvec3 get_possible_target(IN(glm::dvec3) itarget_vector, IN(glm::dvec3) source) noexcept {
	const auto target_cos = sqrt(1.0 - source.z * source.z) - 0.0000001;
	const auto target_sin = sqrt(1.0 - target_cos * target_cos);

	glm::dvec3 oaxis = glm::normalize(glm::cross(itarget_vector, haxis));
	glm::dvec3 lvaxis = glm::cross(oaxis, haxis);

	glm::dvec3 candidates[4] = {
		haxis * target_cos + lvaxis * target_sin,
		haxis * target_cos + lvaxis * -target_sin,
		haxis * -target_cos + lvaxis * target_sin,
		haxis * -target_cos + lvaxis * -target_sin
	};
	int c = 0;
	auto d = glm::dot(candidates[0], itarget_vector);
	if (glm::dot(candidates[1], itarget_vector) > d) {
		d = glm::dot(candidates[1], itarget_vector);
		c = 1;
	}
	if (glm::dot(candidates[2], itarget_vector) > d) {
		d = glm::dot(candidates[2], itarget_vector);
		c = 2;
	}
	if (glm::dot(candidates[3], itarget_vector) > d) {
		d = glm::dot(candidates[3], itarget_vector);
		c = 3;
	}
	return candidates[c];
}

glm::dvec2 project_sphere_to_map(IN(glm::dvec3) source, IN(glm::dvec2) target, INOUT(bool) nosolution) noexcept {
	glm::dvec3 target_vector = sphere_from_globe(globe_from_map(target));

	auto hrotation = solve_trig_polynominal(source.x, source.y, -target_vector.y, nosolution);

	if (nosolution) {
		nosolution = false;
		target_vector = get_possible_target(target_vector, source);
		hrotation = solve_trig_polynominal(source.x, source.y, -target_vector.y, nosolution);
	}

	if (nosolution) {
		return glm::dvec2(0.0, 0.0);
	}

	bool noa_solutions = false;
	const auto h1_sin = sin(hrotation.first);
	const auto h1_cos = cos(hrotation.first);
	const auto vrotation_a = solve_trig_polynominal(source.z, h1_cos * source.x - h1_sin * source.y, -target_vector.x, noa_solutions);
	
	glm::dvec2 best_solution(0.0, 3.14159);

	
		{
			const double z = sin(vrotation_a.first) * (h1_sin * source.y - h1_cos * source.x) + cos(vrotation_a.first) * source.z;
			if(std::signbit(z) == std::signbit(target_vector.z)) {
				if(abs(i_normalize(vrotation_a.first)) < abs(best_solution.y))
					best_solution = glm::dvec2(hrotation.first, i_normalize(vrotation_a.first));
			}
		}

		{
			const double z = sin(vrotation_a.second) * (h1_sin * source.y - h1_cos * source.x) + cos(vrotation_a.second) * source.z;
			if (std::signbit(z) == std::signbit(target_vector.z)) {
				if (abs(i_normalize(vrotation_a.second)) < abs(best_solution.y))
					best_solution = glm::dvec2(hrotation.first, vrotation_a.second);
			}
		}
	

	bool nob_solutions = false;
	const auto h2_sin = sin(hrotation.second);
	const auto h2_cos = cos(hrotation.second);
	const auto vrotation_b = solve_trig_polynominal(source.z, h2_cos * source.x - h2_sin * source.y, -target_vector.x, nob_solutions);

		{
			const double z = sin(vrotation_b.first) * (h2_sin * source.y - h2_cos * source.x) + cos(vrotation_b.first) * source.z;
			if (std::signbit(z) == std::signbit(target_vector.z)) {
				if (abs(i_normalize(vrotation_b.first)) < abs(best_solution.y))
					best_solution = glm::dvec2(hrotation.second, i_normalize(vrotation_b.first));
			}
		}

		{
			const double z = sin(vrotation_b.second) * (h2_sin * source.y - h2_cos * source.x) + cos(vrotation_b.second) * source.z;
			if (std::signbit(z) == std::signbit(target_vector.z)) {
				if (abs(i_normalize(vrotation_b.second)) < abs(best_solution.y))
					best_solution = glm::dvec2(hrotation.second, vrotation_b.second);
			}
		}
	
	
	//if (best_solution.y <= hpi)
	return best_solution;
	
	// nosolution = true;
	// return glm::dvec2(0.0, 0.0);
}

void popualte_border_types(IN(g_lock) l) noexcept {
	edge previous(0, 0);
	int prev_type = -1;

	static flat_set<char_id_t> hos_set;
	static flat_set<admin_id_t> diff_set;
	static bool ch_overlap;

	for (INOUT(auto) bp : global::borders) {
		if (bp.first == previous) {
			bp.second.type = prev_type;
		} else if (bp.first.first > province::last_titled_p || bp.first.second > province::last_titled_p) {
			previous = bp.first;
			prev_type = bp.second.type = -1;
		} else {
			previous = bp.first;

			hos_set.clear();
			diff_set.clear();
			ch_overlap = false;

			global::enum_control_by_prov(prov_id_t(bp.first.first), l, [](IN(controlrecord) r) {
				hos_set.insert(head_of_state(r.ad_controller, fake_lock()));
				diff_set.insert(r.ad_controller);
			});
			global::enum_control_by_prov(prov_id_t(bp.first.second), l, [](IN(controlrecord) r) {
				if (diff_set.count(r.ad_controller) != 0) {
					ch_overlap = true;
					diff_set.erase(r.ad_controller);
				} else {
					diff_set.insert(r.ad_controller);
					if (!ch_overlap && hos_set.count(head_of_state(r.ad_controller, fake_lock())) != 0) {
						ch_overlap = true;
					}
				}
			});

			if (ch_overlap) {
				int dist = 0;
				for (auto a : diff_set) {
					dist = std::max(dist, 5 - get_object(get_object(a, l).associated_title).type);
				}
				prev_type = bp.second.type = dist;
			} else {
				prev_type = bp.second.type = -1;
			}
		}
	}

}

void draw_map( IN(OGLLock) win, double zoom, double xrotation, double yrotation, IN(sf::Texture) terrain) noexcept {
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	//glEnableClientState(GL_VERTEX_ARRAY);

	//glEnable(GL_CULL_FACE);

	glUseProgram(ProgramID);


	glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_FLOAT, 0, NULL);

	
	glProgramUniformMatrix4fv(ProgramID, projection_l, 1, GL_FALSE,  win->getView().getTransform().getMatrix());

	glm::mat4 rotm = generate_mat_transform(xrotation, yrotation);
	glProgramUniformMatrix4fv(ProgramID, rot_l, 1, GL_FALSE, glm::value_ptr(rotm));

#define NUM_FLOATS 3

	glUniform1i(texture_l, 0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	sf::Texture::bind(&terrain);


	for (size_t indx = global::prov_display.size() - 1; indx != 0; --indx) {
		if (global::prov_display[indx].numvertex > 4) {
			glBindBuffer(GL_ARRAY_BUFFER, global::prov_display[indx].vertexbuffer);
			//glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
			glInterleavedArrays(GL_T2F_V3F, 0, nullptr);
			if ((detail::provinces[indx].pflags & PROV_FLAG_HIHGLIGHT) == 0)
				glColor4f(global::prov_display[indx].red, global::prov_display[indx].green, global::prov_display[indx].blue, 1.0f); //glProgramUniform4f(ProgramID, ccolor_l, global::provinces[indx].red, global::provinces[indx].green, global::provinces[indx].blue, 1.0f);
			else
				glColor4f(global::prov_display[indx].red*0.5f, global::prov_display[indx].green*0.5f, global::prov_display[indx].blue*0.5f, 1.0f); //glProgramUniform4f(ProgramID, ccolor_l, global::provinces[indx].red*0.5f, global::provinces[indx].green*0.5f, global::provinces[indx].blue*0.5f, 1.0f);
			glDrawArrays(GL_TRIANGLES, 0, global::prov_display[indx].numvertex);
		}
	}

	sf::Texture::bind(nullptr);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glUseProgram(flat_ProgramID);

	glProgramUniformMatrix4fv(flat_ProgramID, flat_projection_l, 1, GL_FALSE, win->getView().getTransform().getMatrix());
	glProgramUniformMatrix4fv(flat_ProgramID, flat_rot_l, 1, GL_FALSE, glm::value_ptr(rotm));

	/*if (global::focused != 0) {

		glLineWidth(4.0f);

		for (const auto &bdrv : global::borders) {
			for (const auto &bdr : bdrv.second) {
				if (bdr.numvertex >= 2) {
					if (bdrv.first.first == global::focused || bdrv.first.second == global::focused) {
						glColor3f(0.8f, 0.2f, 0.2f);

						glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
						glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
						glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
					}
				}
			}
		}

		glLineWidth(2.0f);

		glBindBuffer(GL_ARRAY_BUFFER, global::provinces[global::focused].vertexbuffer);
		glVertexPointer(NUM_FLOATS, GL_FLOAT, 5*sizeof(GLfloat), NULL);
		glColor3f(0.8f, 0.8f, 0.8f);

		for (size_t i = 0; i < global::provinces[global::focused].numvertex; i += 3) {
			glDrawArrays(GL_LINE_LOOP, i, 3);
		}

		
	}/**/

	//glDisable(GL_CULL_FACE);

	for (const auto &bdrv : global::borders) {
		IN(auto) bdr = bdrv.second;
			if (bdr.numvertex >= 2) {
				if (valid_ids(global::focused) & (global::focused == prov_id_t(bdrv.first.first) | global::focused == prov_id_t(bdrv.first.second))) {
					glColor4f(0.8f, 0.8f, 0.2f, 1.0f);
					glLineWidth(3.5f);
					glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
					glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
					glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
				} else if (!P_HAS_TITLE(bdrv.first.first) || !P_HAS_TITLE(bdrv.first.second)) {
					glLineWidth(static_cast<float>(2.5 * (.7 / zoom)));
					glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
					glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
					glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
					glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
				} else if (bdrv.second.type == 0) {
					glLineWidth(static_cast<float>(1.0 * (.7 / zoom)));
					glColor4f(0.0f, 0.0f, 0.0f, .3f);
					glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
					glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
					glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
				} else if (bdrv.second.type == 1) {
					glLineWidth(static_cast<float>(1.5 * (.7 / zoom)));
					glColor4f(0.0f, 0.0f, 0.0f, .4f);
					glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
					glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
					glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
				} else if (bdrv.second.type == 2) {
					glLineWidth(static_cast<float>(2.0 * (.7 / zoom)));
					glColor4f(0.0f, 0.0f, 0.0f, .5f);
					glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
					glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
					glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
				} else if (bdrv.second.type == 3) {
					glLineWidth(static_cast<float>(2.5 * (.7 / zoom)));
					glColor4f(0.0f, 0.0f, 0.0f, .7f);
					glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
					glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
					glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
				} else if (bdrv.second.type == 4) {
					glLineWidth(static_cast<float>(3.0 * (.7 / zoom)));
					glColor4f(0.0f, 0.0f, 0.0f, .9f);
					glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
					glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
					glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
				} else if (bdrv.second.type == -1) {
					glLineWidth(static_cast<float>(3.0 * (.7 / zoom)));
					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
					glBindBuffer(GL_ARRAY_BUFFER, bdr.vertexbuffer);
					glVertexPointer(NUM_FLOATS, GL_FLOAT, 0, NULL);
					glDrawArrays(GL_LINE_STRIP, 0, bdr.numvertex);
				}
			
		}
	}/**/

	glLineWidth(1.0f);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	glPopClientAttrib();
}

#define BLOCK_SIZE 40

class i_block {
public:
	cvector<std::pair<int, int>> intersects[BLOCK_SIZE];
};

void intersects_to_blocks(v_vector<std::pair<int, int>> &intersects, flat_map<std::pair<int, int>, i_block, std::less<std::pair<int, int>>, concurrent_allocator<std::pair<std::pair<int, int>, i_block>>>& __restrict block_set) noexcept {
	for (size_t it = 0; it < intersects.index.size(); ++it) {
		int previous = -10;
		const auto bound = intersects.get_row(static_cast<unsigned int>(it));
		for (auto iit = bound.first; iit != bound.second; ++iit) {
			if (previous == -10) {
				auto& blk = block_set[std::pair<int, int>(iit->first / BLOCK_SIZE, static_cast<int>(it) / BLOCK_SIZE)];
				blk.intersects[it % BLOCK_SIZE].push_back(*iit);
				previous = iit->first / BLOCK_SIZE;
			} else {
				int xblk = iit->first / BLOCK_SIZE;
				if (xblk == previous) {
					auto& blk = block_set[std::pair<int, int>(xblk, static_cast<int>(it) / BLOCK_SIZE)];
					blk.intersects[it % BLOCK_SIZE].push_back(*iit);
					previous = -10;
				} else {
					auto& prv = block_set[std::pair<int, int>(previous, static_cast<int>(it) / BLOCK_SIZE)];
					prv.intersects[it % BLOCK_SIZE].push_back(std::pair<int, int>(previous * BLOCK_SIZE + BLOCK_SIZE, 0));
					for (int i = previous + 1; i < xblk; ++i) {
						auto& blk = block_set[std::pair<int, int>(i, static_cast<int>(it) / BLOCK_SIZE)];
						blk.intersects[it % BLOCK_SIZE].push_back(std::pair<int, int>(i*BLOCK_SIZE, 0));
						blk.intersects[it % BLOCK_SIZE].push_back(std::pair<int, int>(i*BLOCK_SIZE + BLOCK_SIZE, 0));
					}
					auto& blk = block_set[std::pair<int, int>(xblk, static_cast<int>(it) / BLOCK_SIZE)];
					blk.intersects[it % BLOCK_SIZE].push_back(std::pair<int, int>(xblk * BLOCK_SIZE, 0));
					blk.intersects[it % BLOCK_SIZE].push_back(*iit);
					previous = -10;
				}
			}
		}
	}

}

class i_polyline {
public:
	std::list<std::pair<int, int>, concurrent_allocator<std::pair<int, int>>> points;
};

bool in_polyline_list(IN(cvector<i_polyline>) plines, IN(std::pair<int, int>) in) noexcept {
	for (const auto& pl : plines) {
		if (std::find(pl.points.cbegin(), pl.points.cend(), in) != pl.points.cend())
			return true;
	}
	return false;
}


void blocks_to_polylines(int top, const i_block& block, INOUT(cvector<i_polyline>) plines) noexcept {
	int previoussize = 0;
	cvector<i_polyline> generatedlines;

	for (size_t vertindex = 0; vertindex < BLOCK_SIZE; ++vertindex) {
		bool testcontinuity = false;
		if (previoussize == (((block.intersects[vertindex].size() + 1) / 2) << 1)) {
			int indx = 0;
			for (const auto& pt : block.intersects[vertindex]) {
				if (indx % 2 == 0) {
					if (pt.first < generatedlines[indx / 2].points.front().first - 1 || pt.first > generatedlines[indx / 2].points.front().first + 1) {
						testcontinuity = true;
						break;
					}
				} else {
					if (pt.first < generatedlines[indx / 2].points.back().first - 1 || pt.first > generatedlines[indx / 2].points.back().first + 1) {
						testcontinuity = true;
						break;
					}
				}
				++indx;
			}
		}

		if (testcontinuity || previoussize != (((block.intersects[vertindex].size() + 1) / 2) << 1)) {
			if (previoussize != 0) {
				int indx = 0;
				for (const auto& pt : block.intersects[vertindex - 1]) {
					if (indx % 2 == 0)
						generatedlines[indx / 2].points.push_front(std::pair<int, int>(pt.first + pt.second, static_cast<int>(vertindex) + top));
					else
						generatedlines[indx / 2].points.push_back(std::pair<int, int>(pt.first + pt.second, static_cast<int>(vertindex) + top));
					++indx;
				}
				for (auto& gl : generatedlines)
					plines.emplace_back(std::move(gl));
				generatedlines.clear();
			}
			generatedlines.resize((block.intersects[vertindex].size() + 1) / 2);
			previoussize = static_cast<int>(((block.intersects[vertindex].size() + 1) / 2) << 1);
		}

		int indx = 0;
		for (const auto& pt : block.intersects[vertindex]) {
			if (indx % 2 == 0) {
				generatedlines[indx / 2].points.push_front(std::pair<int, int>(pt.first, static_cast<int>(vertindex) + top));
				generatedlines[indx / 2].points.push_front(std::pair<int, int>(pt.first + pt.second, static_cast<int>(vertindex + 1) + top));
			} else {
				generatedlines[indx / 2].points.push_back(std::pair<int, int>(pt.first, static_cast<int>(vertindex) + top));
				generatedlines[indx / 2].points.push_back(std::pair<int, int>(pt.first + pt.second, static_cast<int>(vertindex + 1) + top));
			}
			++indx;
		}
	}

	if (previoussize != 0) {
		int indx = 0;
		for (const auto& pt : block.intersects[BLOCK_SIZE - 1]) {
			if (indx % 2 == 0)
				generatedlines[indx / 2].points.push_front(std::pair<int, int>(pt.first + pt.second, BLOCK_SIZE + top));
			else
				generatedlines[indx / 2].points.push_back(std::pair<int, int>(pt.first + pt.second, BLOCK_SIZE + top));
			++indx;
		}
		for (auto& gl : generatedlines)
			plines.emplace_back(std::move(gl));
	}
}


void reduce_polyline(i_polyline& __restrict ln) noexcept {
	if (ln.points.size() < 4)
		return;

	bool up = true;
	auto it = ln.points.begin();
	int lastx = it->first;
	int lasty = it->second;
	int slopex = BLOCK_SIZE * 2;
	int slopey = BLOCK_SIZE * 2;

	++it;
	while (it != ln.points.end()) {
		if (it->first == lastx && it->second == lasty) {
			--it;
			it = ln.points.erase(it);
		} else if (up && it->second < lasty) {
			up = false;
		} else if (it->first - lastx == slopex && it->second - lasty == slopey) {
			--it;
			it = ln.points.erase(it);
		} else {
			slopex = it->first - lastx;
			slopey = it->second - lasty;
		}

		lasty = it->second;
		lastx = it->first;
		++it;
	}
}

int inline triangle_sign(int x1, int y1, int x2, int y2, int x3, int y3) noexcept {
	return (x1*y2 - x2*y1 + x2*y3 - x3*y2 + x3*y1 - x1*y3);
}


void monotone_to_triangles(cvector<sf::Vector2f>& __restrict pts, i_polyline& __restrict ln) noexcept {
	if (ln.points.size() < 3)
		return;

	/*for(const auto& p : ln.points)
	pts.emplace_back(static_cast<float>(p.first), static_cast<float>(p.second));
	return;/**/

	struct pointrecord {
		std::pair<int, int> pt;
		bool left;
		pointrecord(const std::pair<int, int> p, bool l) : pt(p), left(l) {}
	};

	std::vector<pointrecord> sorted;
	bool left = true;
	int lasty = ln.points.front().second;
	auto it = ln.points.begin();
	auto rit = it;
	for (; it != ln.points.end(); ++it) {
		if (it->second > lasty) {
			--it;
			rit = it;
			break;
		}
		lasty = it->second;
	}

	auto lit = std::list<std::pair<int, int>>::reverse_iterator(rit);
	while (lit != ln.points.rend() && rit != ln.points.end()) {
		if (lit->second <= rit->second) {
			sorted.emplace_back(*lit, true);
			++lit;
		} else {
			sorted.emplace_back(*rit, false);
			++rit;
		}
	}

	while (lit != ln.points.rend()) {
		sorted.emplace_back(*lit, true);
		++lit;
	}

	while (rit != ln.points.end()) {
		sorted.emplace_back(*rit, false);
		++rit;
	}

	/*for (const auto& pt : ln.points) {
	if (pt.second > lasty) {
	left = false;
	sorted.back().left = false;
	}
	lasty = pt.second;
	sorted.emplace_back(pt, left);
	}

	std::sort(sorted.begin(), sorted.end(), [](const pointrecord& a, const pointrecord& b) {return a.pt.second <= b.pt.second; });/**/


	std::vector<pointrecord> stack;
	size_t indx = 2;
	stack.push_back(sorted[0]);
	stack.push_back(sorted[1]);

	while (indx < sorted.size()) {
		if (stack.back().left != sorted[indx].left) {
			pointrecord t = stack.back();

			if (sorted[indx].left) {
				while (stack.size() >= 2) {
					pts.emplace_back(static_cast<float>(sorted[indx].pt.first), static_cast<float>(sorted[indx].pt.second));
					pts.emplace_back(static_cast<float>(stack.back().pt.first), static_cast<float>(stack.back().pt.second));
					stack.pop_back();
					pts.emplace_back(static_cast<float>(stack.back().pt.first), static_cast<float>(stack.back().pt.second));
				}
			} else {
				while (stack.size() >= 2) {
					pts.emplace_back(static_cast<float>(stack.back().pt.first), static_cast<float>(stack.back().pt.second));
					pts.emplace_back(static_cast<float>(sorted[indx].pt.first), static_cast<float>(sorted[indx].pt.second));
					stack.pop_back();
					pts.emplace_back(static_cast<float>(stack.back().pt.first), static_cast<float>(stack.back().pt.second));
				}
			}
			stack.pop_back();
			stack.push_back(t);
			stack.push_back(sorted[indx]);
		} else {
			if (sorted[indx].left) {
				while (stack.size() >= 2) {
					pointrecord t = stack.back();
					stack.pop_back();

					if (triangle_sign(sorted[indx].pt.first, sorted[indx].pt.second, t.pt.first, t.pt.second, stack.back().pt.first, stack.back().pt.second) > 0) {
						pts.emplace_back(static_cast<float>(t.pt.first), static_cast<float>(t.pt.second));
						pts.emplace_back(static_cast<float>(sorted[indx].pt.first), static_cast<float>(sorted[indx].pt.second));
						pts.emplace_back(static_cast<float>(stack.back().pt.first), static_cast<float>(stack.back().pt.second));
					} else {
						stack.push_back(t);
						break;
					}
				}
				stack.push_back(sorted[indx]);
			} else {
				while (stack.size() >= 2) {
					pointrecord t = stack.back();
					stack.pop_back();

					if (triangle_sign(sorted[indx].pt.first, sorted[indx].pt.second, t.pt.first, t.pt.second, stack.back().pt.first, stack.back().pt.second) < 0) {
						pts.emplace_back(static_cast<float>(stack.back().pt.first), static_cast<float>(stack.back().pt.second));
						pts.emplace_back(static_cast<float>(sorted[indx].pt.first), static_cast<float>(sorted[indx].pt.second));
						pts.emplace_back(static_cast<float>(t.pt.first), static_cast<float>(t.pt.second));
					} else {
						stack.push_back(t);
						break;
					}
				}
				stack.push_back(sorted[indx]);
			}
		}
		++indx;
	}
}

void intersects_to_triangles(INOUT(v_vector<std::pair<int, int>>) intersects, int top, INOUT(cvector<sf::Vector2f>) pts) noexcept {
	flat_map<std::pair<int, int>, i_block, std::less<std::pair<int, int>>, concurrent_allocator<std::pair<std::pair<int, int>, i_block> >> block_set;
	intersects_to_blocks(intersects, block_set);
	cvector<i_polyline> plines;
	for (IN(auto) bk : block_set) {
		blocks_to_polylines(top + bk.first.second * BLOCK_SIZE, bk.second, plines);
	}
	for (INOUT(auto) pl : plines) {
		reduce_polyline(pl);
		monotone_to_triangles(pts, pl);
	}
}

void generic_color_province(INOUT(province_display) pd, decltype(declval<province>().pflags) flag) noexcept {
	if ((flag & province::FLAG_WATER) != 0) {
		pd.red = 0.15f;
		pd.green = 0.15f;
		pd.blue = 0.8f;
	} else {
		pd.red = 0.3f;
		pd.green = 0.3f;
		pd.blue = 0.3f;
	}
}

void color_prov_by_distance(prov_id_t prov_from) noexcept {
	IN(auto) pf = get_object(prov_from);

	const auto pz = global::prov_display.size();
	for (prov_id i = 1; i != pz; ++i) {
		if (P_HAS_TITLE(i)) {
			INOUT(auto) pd = global::prov_display[i];
			double distance = i != static_cast<size_t>(prov_from.value) ? province_distance(detail::provinces[i], pf) : 0.0;
			if (distance > 600.0) {
				pd.red = 0.0f;
				pd.green = 0.0f;
				pd.blue = 0.0f;
			} else {
				pd.red = 0.0f;
				pd.green = static_cast<GLfloat>((600.0 - distance) / 600.0);
				pd.blue = 0.0f;
			}
			size_t param = static_cast<size_t>(distance);
			global::provtooltip[prov_id_t(i)] = wstr_to_str(get_p_string(TX_L_DISTANCE, &param, 1));
		} else {
			generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
		}
	}
}



void color_prov_by_culture(cul_id_t selected) noexcept {
	const auto pz = global::prov_display.size();
	for (prov_id i = 1; i != pz; ++i) {
		const auto culture = detail::provinces[i].culture;
		if (culture == selected) {
			INOUT(auto) pd = global::prov_display[i];
			pd.red = 0.0f;
			pd.green = 1.0f;
			pd.blue = 0.0f;
		} else {
			generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
		}
		global::provtooltip[prov_id_t(i)] = get_object(culture).name.get();
	}
}

void color_prov_by_religion(rel_id_t selected) noexcept {
	const auto pz = global::prov_display.size();
	for (prov_id i = 1; i != pz; ++i) {
		const auto religion = detail::provinces[i].religion;
		if (selected == religion) {
			INOUT(auto) pd = global::prov_display[i];
			pd.red = 0.0f;
			pd.green = 1.0f;
			pd.blue = 0.0f;
		} else {
			generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
		}
		global::provtooltip[prov_id_t(i)] = get_object(religion).name.get();
	}
}

void color_prov_by_vassal(admin_id_t leige, IN(sf::Texture) cap, IN(g_lock) l) noexcept {
	const auto pz = global::prov_display.size();
	for (prov_id i = 1; i != pz; ++i) {

		if (P_HAS_TITLE(i)) {
			const administration* adm = nullptr;

			global::enum_control_by_prov(prov_id_t(i), fake_lock(), [&adm, leige](IN(controlrecord) r) {
				IN(auto) nadm = get_object(r.ad_controller, fake_lock());
				if (nadm.leige == leige || (!valid_ids(nadm.leige) && adm == nullptr)) {
					adm = &nadm;
				}
			});

			if (adm) {
				INOUT(auto) pd = global::prov_display[i];
				IN(auto) t = get_object(adm->associated_title);

				pd.red = static_cast<float>(t.color1.r) / 256.0f;
				pd.green = static_cast<float>(t.color1.g) / 256.0f;
				pd.blue = static_cast<float>(t.color1.b) / 256.0f;

				global::provtooltip[prov_id_t(i)] = global::title_name(adm->associated_title);

				if (adm->capital == prov_id_t(i)) {
					global::mapsprites.emplace_back(cap);
					global::mapsprites.back().setOrigin(14, 14);
					const auto pos = sphere_to_texture(detail::provinces[i].centroid);
					global::mapsprites.back().setPosition(static_cast<float>(pos.x), static_cast<float>(pos.y));
				}
			} else {
				generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
			}
		} else {
			generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
		}
	}

	
}

void color_prov_by_title(IN(sf::Texture) cap, IN(g_lock) l) noexcept {
	const auto pz = global::prov_display.size();
	for (prov_id i = 1; i != pz; ++i) {

		if (P_HAS_TITLE(i)) {
			const administration* adm = nullptr;

			global::enum_control_by_prov(prov_id_t(i), fake_lock(), [&adm](IN(controlrecord) r) {
				IN(auto) nadm = get_object(r.ad_controller, fake_lock());
				if(!valid_ids(nadm.leige))
					adm = &nadm;
			});


			if (adm) {
				INOUT(auto) pd = global::prov_display[i];
				IN(auto) t = get_object(adm->associated_title);

				pd.red = static_cast<float>(t.color1.r) / 256.0f;
				pd.green = static_cast<float>(t.color1.g) / 256.0f;
				pd.blue = static_cast<float>(t.color1.b) / 256.0f;

				global::provtooltip[prov_id_t(i)] = global::title_name(adm->associated_title);

				if (adm->capital == prov_id_t(i)) {
					global::mapsprites.emplace_back(cap);
					global::mapsprites.back().setOrigin(14, 14);
					const auto pos = sphere_to_texture(detail::provinces[i].centroid);
					global::mapsprites.back().setPosition(static_cast<float>(pos.x), static_cast<float>(pos.y));
				}
			} else {
				generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
			}
		} else {
			generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
		}
	}

}

void color_prov_by_tax() noexcept {
	const auto pz = global::prov_display.size();
	for (prov_id i = 1; i != pz; ++i) {
		if (P_HAS_TITLE(i)) {
			INOUT(auto) pd = global::prov_display[i];

			pd.red = 0.0f;
			pd.green = detail::provinces[i].tax > 1.5 ? 1.0f : static_cast<float>(detail::provinces[i].tax / 1.5);
			pd.blue = 0.0f;

			size_t param = to_param(detail::provinces[i].tax);
			global::provtooltip[prov_id_t(i)] = wstr_to_str(get_p_string(TX_MAP_TAX_LABEL, &param, 1));
		} else {
			generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
		}
	}
}

void clear_map_display() noexcept {
	const auto pz = global::prov_display.size();
	for (size_t i = 1; i != pz; ++i) {
		generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
	}
}

void color_pov_in_war(war_id_t w, prov_id_t p, float sr, float sg, float sb, float or , float og, float ob, IN(sf::Texture) seige_ico, IN(g_lock) l) {
	INOUT(auto) pd = global::prov_display[p.value];
	if (!occupation_info.contains(p, l)) {
		pd.red = sr;
		pd.green = sg;
		pd.blue = sb;
	} else {
		IN(auto) oi = occupation_info.get(p, l);
		if (oi.in_war == w) {
			pd.red = or;
			pd.green = og;
			pd.blue = ob;
			if (oi.since != 0) {
				global::mapsprites.emplace_back(seige_ico);
				global::mapsprites.back().setOrigin(18, 18);
				const auto pos = sphere_to_texture(get_object(p).centroid);
				global::mapsprites.back().setPosition(static_cast<float>(pos.x), static_cast<float>(pos.y));
			}
		} else {
			pd.red = 0.75f;
			pd.green = 0.75f;
			pd.blue = 0.75f;
		}
	}
}

void display_nowar(IN(g_lock) l) noexcept {
	const auto pz = global::prov_display.size();
	for (prov_id i = 1; i != pz; ++i) {
		if (occupation_info.contains(prov_id_t(i), l)) {
			INOUT(auto) pd = global::prov_display[i];
			pd.red = 0.5f;
			pd.green = 0.5f;
			pd.blue = 0.5f;
		} else {
			generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
		}
	}
}


void displaywar(war_id_t w, bool attacker, IN(sf::Texture) seige_ico, IN(sf::Texture) target_ico, IN(g_lock) l) noexcept {
	IN(auto) wr = war_pool.get(w.value, l);

	cflat_set<admin_id_t> all_attackers;
	cflat_set<admin_id_t> all_defenders;

	list_participants(wr.attacker, all_attackers, l);
	list_participants(wr.defender, all_defenders, l);

	clear_map_display();

	cvector<prov_id_t> acontrolled;
	cvector<prov_id_t> bcontrolled;

	for (auto a : all_attackers)
		controlled_by_admin(a, acontrolled, l);
	for (auto a : all_defenders)
		controlled_by_admin(a, bcontrolled, l);

	const auto pz = global::prov_display.size();
	for (prov_id i = 1; i != pz; ++i) {
		if (occupation_info.contains(prov_id_t(i), l)) {
			INOUT(auto) pd = global::prov_display[i];
			pd.red = 0.5f;
			pd.green = 0.5f;
			pd.blue = 0.5f;
		} else {
			generic_color_province(global::prov_display[i], detail::provinces[i].pflags);
		}
		
	}
	if (attacker) {
		for (auto p : acontrolled) {
			color_pov_in_war(w, p, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f, 0.0f, target_ico, l);
		}
		for (auto p : bcontrolled) {
			color_pov_in_war(w, p, 1.0f, 0.0f, 0.0f, 0.9f, 0.9f, 0.0f, target_ico, l);
		}
	} else {
		for (auto p : acontrolled) {
			color_pov_in_war(w, p, 1.0f, 0.0f, 0.0f, 0.9f, 0.9f, 0.0f, target_ico, l);
		}
		for (auto p : bcontrolled) {
			color_pov_in_war(w, p, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f, 0.0f, target_ico, l);
		}
	}
}