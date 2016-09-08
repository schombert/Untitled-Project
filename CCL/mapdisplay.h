#pragma once
#include "globalhelpers.h"
#include "structs.hpp"

void create_province_program() noexcept;
void load_display_data(IN(std::vector<cvector<sf::Vector2f>>) quads, IN(flat_multimap<edge, std::vector<sf::Vector2<short>>>) brder, IN(OGLLock) win) noexcept;
void fake_load_display_data(IN(flat_multimap<edge, std::vector<sf::Vector2<short>>>) brder) noexcept;
void popualte_border_types(IN(g_lock) l) noexcept;

void draw_map( IN(OGLLock) win, double zoom, double xrotation, double yrotation, IN(sf::Texture) terrain) noexcept;

glm::dvec2 map_to_globe(IN(glm::dvec2) source, IN(glm::dmat4) inverse_rotation) noexcept;
glm::dmat4 generate_mat_transform(const double horz_rotation, const double vert_rotation) noexcept;
glm::dmat4 generate_mat_inverse_transform(const double horz_rotation, const double vert_rotation) noexcept;
glm::dvec2 map_to_texture(IN(glm::dvec2) source, IN(glm::dmat4) inverse_rotation) noexcept;
glm::dvec2 texture_to_map(IN(glm::dvec2) source, IN(glm::dmat4) rotation) noexcept;
glm::dvec3 texture_to_sphere(IN(glm::dvec2) source) noexcept;
glm::dvec2 globe_from_texture(IN(glm::dvec2) source) noexcept;
glm::dvec3 sphere_from_globe(IN(glm::dvec2) source) noexcept;
glm::dvec2 globe_from_map(IN(glm::dvec2) source) noexcept;
glm::dvec2 texture_from_globe(IN(glm::dvec2) source) noexcept;
glm::dvec2 map_to_globe(IN(glm::dvec2) source, IN(glm::dmat4) inverse_rotation) noexcept;
glm::dvec2 map_from_globe(IN(glm::dvec2) source) noexcept;
glm::dvec2 texture_to_globe(IN(glm::dvec2) source, IN(glm::dmat4) rotation) noexcept;
glm::dvec2 sphere_to_map(IN(glm::dvec3) source, IN(glm::dmat4) rotation) noexcept;
glm::dvec2 sphere_to_globe(IN(glm::dvec3) source, IN(glm::dmat4) rotation) noexcept;
glm::dvec2 sphere_to_texture(IN(glm::dvec3) source) noexcept;
glm::dvec3 map_to_sphere(IN(glm::dvec2) source, IN(glm::dmat4) inverse_rotation) noexcept;
double distance_from_globe(IN(glm::dvec2) a, IN(glm::dvec2) b) noexcept;
double distance_from_texture(IN(glm::dvec2) a, IN(glm::dvec2) b) noexcept;
double distance_from_map(IN(glm::dvec2) a, IN(glm::dvec2) b) noexcept;
double province_distance(IN(province) a, IN(province) b) noexcept;
// half_fp hp_distance_from_texture(IN(glm::tvec2<half_fp>) a, IN(glm::tvec2<half_fp>) b) noexcept;
// half_fp hp_distance_from_map(IN(glm::tvec2<half_fp>) a, IN(glm::tvec2<half_fp>) b) noexcept;
// half_fp hp_distance_from_globe(IN(glm::tvec2<half_fp>) a, IN(glm::tvec2<half_fp>) b) noexcept;
glm::dvec2 project_sphere_to_map(IN(glm::dvec3) source, IN(glm::dvec2) target, INOUT(bool) nosolution) noexcept;
glm::dvec2 nr_step(glm::dvec2 current_step, glm::dvec3 source, glm::dvec3 target) noexcept;

void intersects_to_triangles(INOUT(v_vector<std::pair<int, int>>) intersects, int top, INOUT(cvector<sf::Vector2f>) pts) noexcept;
void generic_color_province(INOUT(province_display) pd, decltype(std::declval<province>().pflags) flag) noexcept;
void color_prov_by_distance(prov_id_t prov_from) noexcept;
void color_prov_by_culture(cul_id_t selected) noexcept;
void color_prov_by_religion(rel_id_t selected)  noexcept;
void color_prov_by_vassal(admin_id_t leige, IN(sf::Texture) cap, IN(g_lock) l) noexcept;
void color_prov_by_title(IN(sf::Texture) cap, IN(g_lock) l) noexcept;
void color_prov_by_tax() noexcept;
void displaywar(war_id_t w, bool attacker, IN(sf::Texture) seige_ico, IN(sf::Texture) target_ico, IN(g_lock) l) noexcept;
void display_nowar(IN(g_lock) l) noexcept;
void clear_map_display() noexcept;