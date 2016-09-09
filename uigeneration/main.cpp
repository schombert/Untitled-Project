#include "globalhelpers.h"
#include "fileparsing.h"

#define DEF_SPACING "5"


class uie {
public:
	std::string name;
	std::string type;
	std::string action;
	std::string text;
	std::string tooltip;
	std::string tooltipformat;
	std::string textformat;
	std::string contentfunction;
	std::string contentlist;
	std::string contentvector;
	std::string start;
	std::string end;
	std::string position;
	std::string toggle;
	std::vector<std::string> object;
	bool isclass = false;
	bool isstruct = false;
	std::vector<std::string> paint;
	std::vector<std::string> parameters;
	std::vector<std::unique_ptr<uie>> subelements;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool static_obj = true;
};


std::unique_ptr<uie> parse_ui_element(IN_P(prse_list) def) {
	std::unique_ptr<uie> result = std::make_unique<uie>();
	result->name = def->value;

	for (IN(auto) e : def->list) {
		if (e.value) {
			if (e.value->value == "static")
				result->static_obj = true;
			else if (e.value->value == "dynamic")
				result->static_obj = false;
		} else if (e.assoc) {
			if (e.assoc->left == "w") {
				result->width = std::stol(e.assoc->right.c_str());
			} else if (e.assoc->left == "h") {
				result->height = std::stol(e.assoc->right.c_str());
			} else if (e.assoc->left == "x") {
				result->x = std::stol(e.assoc->right.c_str());
			} else if (e.assoc->left == "y") {
				result->y = std::stol(e.assoc->right.c_str());
			} else if (e.assoc->left == "type") {
				result->type = e.assoc->right;
			} else if (e.assoc->left == "object" || e.assoc->left == "objects") {
				result->object.push_back(e.assoc->right);
			} else if (e.assoc->left == "action") {
				result->action = e.assoc->right;
			} else if (e.assoc->left == "text") {
				result->text = e.assoc->right;
			} else if (e.assoc->left == "textformat") {
				result->textformat = e.assoc->right;
			} else if (e.assoc->left == "tooltipformat") {
				result->textformat = e.assoc->right;
			} else if (e.assoc->left == "tooltip") {
				result->tooltip = e.assoc->right;
			} else if (e.assoc->left == "fill") {
				result->paint.push_back(e.assoc->right);
			} else if (e.assoc->left == "contentfunction" || e.assoc->left == "function" || e.assoc->left == "if") {
				result->contentfunction = e.assoc->right;
			} else if (e.assoc->left == "start" || e.assoc->left == "begin") {
				result->start = e.assoc->right;
			} else if (e.assoc->left == "end" || e.assoc->left == "finish") {
				result->end = e.assoc->right;
			} else if (e.assoc->left == "position" || e.assoc->left == "pos") {
				result->position = e.assoc->right;
			} else if (e.assoc->left == "iteration") {
				result->contentlist = e.assoc->right;
			} else if (e.assoc->left == "toggle") {
				result->toggle = e.assoc->right;
			} else if (e.assoc->left == "vector") {
				result->contentvector = e.assoc->right;
			} else if (e.assoc->left == "param" || e.assoc->left == "params") {
				result->parameters.push_back(e.assoc->right);
			}
		} else if (e.list) {
			if (e.list->value == "fill") {
				for (IN(auto) f : e.list->list) {
					if (f.value)
						result->paint.push_back(f.value->value);
				}
			} else if (e.list->value == "params" || e.list->value == "param") {
				for (IN(auto) f : e.list->list) {
					if (f.value)
						result->parameters.push_back(f.value->value);
				}
			} else if (e.list->value == "object" || e.list->value == "objects") {
				for (IN(auto) f : e.list->list) {
					if (f.value)
						result->object.push_back(f.value->value);
				}
			} else {
				result->subelements.emplace_back(std::move(parse_ui_element(e.list.get())));
			}
		}
	}
	return result;
}


std::vector<std::unique_ptr<uie>> top_lvl_containers;

void auto_name(IN(std::vector<std::unique_ptr<uie>>) v, size_t& count) {
	for (IN(auto) e : v) {
		if (e->name.length() == 0) {
			e->name = std::string("e_") + std::to_string(count++);
		}
		auto_name(e->subelements, count);
	}
}

std::vector<std::string> extras;

void fix_params(IN(std::vector<std::unique_ptr<uie>>) v) {
	for (IN(auto) e : v) {
		for (INOUT(auto) s : e->parameters) {
			if (s[0] == '.')
				s = "obj" + s;
		}
		if (e->contentlist.length() != 0) {
			if (e->contentlist[0] == '.') {
				e->contentlist = std::string("obj") + e->contentlist;
			}
		}
		if (e->contentfunction.length() != 0) {
			if (e->contentfunction[0] == '.') {
				e->contentfunction = std::string("obj") + e->contentfunction;
			}
		}
		if (e->start.length() != 0) {
			if (e->start[0] == '.') {
				e->start = std::string("obj") + e->start;
			}
		}
		if (e->end.length() != 0) {
			if (e->end[0] == '.') {
				e->end = std::string("obj") + e->end;
			}
		}
		if (e->toggle.length() != 0) {
			if (e->toggle[0] == '.') {
				e->toggle = std::string("obj") + e->toggle;
			}
		}
		for (INOUT(auto) s : e->object) {
			if (s.length() != 0) {
				if (s.substr(0, 6) == "class ") {
					extras.push_back(s + ";");
					s = s.substr(6, std::string::npos);
				} else if (s.substr(0, 7) == "struct ") {
					extras.push_back(s + ";");
					s = s.substr(7, std::string::npos);
				} else if (s[0] == '.') {
					s = std::string("obj") + s;
				}
			}
		}
		
		fix_params(e->subelements);
	}
}

void default_values(IN(std::vector<std::unique_ptr<uie>>) v) {
	for (IN(auto) e : v) {
		if (e->type == "uiCountDown") {
			if (e->textformat.length() == 0) {
				e->textformat = "tooltip_text";
			}
			if (e->paint.size() == 0) {
				e->paint.emplace_back("bar_fill");
			}
			while (e->parameters.size() < 2) {
				e->parameters.emplace_back("0");
			}
		} else if (e->type == "uiBar") {
			if (e->textformat.length() == 0) {
				e->textformat = "tooltip_text";
			}
			if (e->paint.size() == 0) {
				e->paint.emplace_back("bar_fill");
			}
			if (e->parameters.size() == 0) {
				e->parameters.emplace_back("0");
			}
		} else if (e->type == "text" || e->type == "uiHozContainer" || e->type == "uiScrollView") {
			if (e->textformat.length() == 0) {
				e->textformat = "standard_text";
			}
			if (e->paint.size() == 0) {
				e->paint.emplace_back("empty");
			}
		} else if (e->type == "uiTabs") {
			if (e->textformat.length() == 0) {
				e->textformat = "header_text";
			}
			if (e->paint.size() == 0) {
				e->paint.emplace_back("solid_border");
			}
		} else if (e->type == "function") {
			if (e->object.size() == 0)
				e->object.emplace_back("obj");
		} else if (e->type == "ui_button_disable" || e->type == "ui_toggle_button") {
			if (e->textformat.length() == 0) {
				e->textformat = "standard_text";
			}
			if (e->paint.size() == 0) {
				e->paint.emplace_back("solid_border");
			}
			if (e->paint.size() == 1) {
				e->paint.emplace_back("disabled_fill");
			}
			if (e->tooltipformat.length() == 0) {
				e->tooltipformat = "tooltip_text";
			}
		}  else {
			if (e->textformat.length() == 0) {
				e->textformat = "standard_text";
			}
			if (e->paint.size() == 0) {
				e->paint.emplace_back("solid_border");
			}
			if (e->tooltipformat.length() == 0) {
				e->tooltipformat = "tooltip_text";
			}
		}
		default_values(e->subelements);
	}
}

void write_tb_string(HANDLE file, IN(std::string) str, int tabs) {
	std::string towrite = std::string(tabs, '\t') + str + "\r\n";
	WriteFile(file, towrite.c_str(), static_cast<DWORD>(towrite.length()), nullptr, nullptr);
}

bool perm_storable(IN(uie) e) {
	if (e.type == "uiBar" && e.parameters.size() == 1)
		return e.static_obj;
	if (e.parameters.size() != 0)
		return false;
	if (e.type == "list" || e.type == "conditional" || e.type == "character_selection" || e.type == "function" || e.type == "modal_button")
		return false;
	if (e.type == "uiButton" || e.type == "ui_button_disable" || e.type == "uiCheckBox" || e.type == "uiCountDown" || e.type == "uiGButton" || e.type == "ui_toggle_button"
		|| e.type == "large_tbutton" || e.type == "small_tbutton" || e.type == "large_chbutton" || e.type == "small_chbutton")
		return e.static_obj;
	return true;
}

bool can_store(IN(uie) e) {
	if (e.type == "text" && e.parameters.size() != 0)
		return false;
	if (e.type == "list" || e.type == "conditional" || e.type == "character_selection" || e.type == "function")
		return false;
	return true;
}

std::string storage_type(IN(uie) e) {
	if (e.type == "window") {
		return "uiDragRect";
	} else if (e.type == "modal_window") {
		return "uiDragRect";
	} else if (e.type == "text") {
		return "uiSimpleText";
	} else if (e.type == "character_selection" || e.type == "modal_button") {
		return "uiButton";
	} else if (e.type == "panel" || e.type == "pane" || e.type == "tab" || e.type.length() == 0) {
		return "uiScrollView";
	} else if(e.type == "large_tbutton" || e.type == "small_tbutton" || e.type == "large_chbutton" || e.type == "small_chbutton") {
		return "uiGButton";
	} else {
		return e.type;
	}
}

void write_uiptrs(IN(uie) e, HANDLE file, bool ext, int tabs, IN(std::string) namesp) {
	if (can_store(e)) {
		write_tb_string(file, (ext ? std::string("extern ") : std::string("")) + "std::shared_ptr<" + storage_type(e) + "> " + (ext ? std::string("") : (namesp + "::")) + e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "") + ";", tabs);
	}

	for (IN(auto) p : e.subelements) {
		write_uiptrs(*p, file, ext, tabs + 1, namesp);
	}
}

bool needs_update(IN(uie) e) {
	for (IN(auto) p : e.subelements) {
		if (!perm_storable(*p)) {
			return true;
		}
	}
	return false;
}

void write_param_block_open(IN(uie) e, HANDLE file, int& tabs, size_t offset = 0) {
	if (e.parameters.size() > offset) {
		std::string blockheader = std::string("{ size_t params[") + std::to_string(e.parameters.size() - offset) + "] = {";
		for (size_t i = offset; i != e.parameters.size(); ++i) {
			blockheader += "to_param(";
			blockheader += e.parameters[i];
			blockheader += ")";
			if (i != e.parameters.size() - 1)
				blockheader += ", ";
		}
		blockheader += "};";

		write_tb_string(file, blockheader, tabs);
	} else {
		write_tb_string(file, "{", tabs);
	}

	++tabs;
}

void write_param_block_close(IN(uie) e, HANDLE file, int& tabs, size_t offset = 0) {
	--tabs;
	write_tb_string(file, "}", tabs);
}

void write_wh(IN(uie) e, HANDLE file, IN(std::string) element, IN(std::string) parent, int tabs) {
	write_tb_string(file, std::string("x = ") + element + "->pos.width + " + element + "->pos.left + " DEF_SPACING ";", tabs);
	if (e.type != "text")
		write_tb_string(file, std::string("y = ") + element + "->pos.height + " + element + "->pos.top + " DEF_SPACING ";", tabs);
	else if(e.y >= 0)
		write_tb_string(file, std::string("y += (global::") + e.textformat + ".csize + " DEF_SPACING + (e.y > 0 ? std::string(" + ") + std::to_string(e.y) : std::string("")) + ");", tabs);
	else
		write_tb_string(file, std::string("y = ") + parent + "->pos.height + " + std::to_string(e.y) + " + global::" + e.textformat + ".csize + " DEF_SPACING ");", tabs);
}
std::string extra_params(int totalparams) {
	std::string result;
	for (int i = 1; i != totalparams; ++i) {
		result += "p";
		result += std::to_string(i);
		result += ", ";
	}
	return result;
}

std::string gen_add_string_body(int totalparams, INOUT(uie) e, IN(std::string) parent, bool width = false, bool height = false, bool text = true, size_t params = 0, bool action = false, bool tttext = false, bool ttformat = false, bool textformat = false, bool paint = true) {
	std::string result;

	if (e.x >= 0) {
		result += "x + ";
		result += std::to_string(e.x);
	} else {
		result += parent;
		result += "->pos.width + ";
		result += std::to_string(e.x);
	}
	result += ", ";
	if (e.y >= 0) {
		result += "y + ";
		result += std::to_string(e.y);
	} else {
		result += parent;
		result += "->pos.height + ";
		result += std::to_string(e.y);
	}
	if (width) {
		result += ", ";
		if (e.width > 0) {
			result += std::to_string(e.width);
		} else if (e.x >= 0) {
			result += parent;
			result += "->pos.width - (x + ";
			result += std::to_string(e.x);
			result += ") + ";
			result += std::to_string(e.width);
		} else {
			result += std::to_string(e.width);
			result += " - ";
			result += std::to_string(e.x);
		}
	}
	if (height) {
		result += ", ";
		if (e.height > 0) {
			result += std::to_string(e.height);
		} else if (e.y >= 0) {
			result += parent;
			result += "->pos.height - (y + ";
			result += std::to_string(e.y);
			result += ") + ";
			result += std::to_string(e.height);
		} else {
			result += std::to_string(e.height);
			result += " - ";
			result += std::to_string(e.y);
		}
	}
	if (text) {
		if (params != 0) {
			result += ", get_p_string(TX_";
			result += e.text;
			result += ", params, ";
			result += std::to_string(params);
			result += ")";
		} else {
			result += ", get_simple_string(TX_";
			result += e.text;
			result += ")";
		}
	}
	if (paint) {
		if (e.paint.size() == 1) {
			result += ", global::";
			result += e.paint[0];
		} else if (e.paint.size() > 1) {
			result += ", paint_states<";
			result += std::to_string(e.paint.size());
			result += ">(";
			for (size_t i = 0; i != e.paint.size() - 1; ++i) {
				result += "global::";
				result += e.paint[i];
				result += ", ";
			}
			result += "global::";
			result += e.paint[e.paint.size() - 1];
			result += ")";
		}
	}
	if (text || textformat) {
		result += ", global::";
		result += e.textformat;
	}
	if (tttext && e.tooltip.length() != 0) {
		result += ", get_simple_string(TX_";
		result += e.tooltip;
		result += ")";
	}
	if (tttext || ttformat) {
		result += ", global::";
		result += e.tooltipformat;
	}
	if (action) {
		if (e.type == "modal_button") {
			result += ", ";
			result += e.name;
			result += "_action(obj, ";
			result += extra_params(totalparams);
			result += "signal, l)";
		} else if (e.static_obj) {
			result += ", ";
			result += e.name;
			result += "_action";
		} else {
			result += ", ";
			result += e.name;
			result += "_action(obj, ";
			result += extra_params(totalparams);
			result += "l)";
		}
	}
	return result;
}

std::string gen_add_string(int totalparams, INOUT(uie) e, IN(std::string) parent, bool width = false, bool height = false, bool text = true, size_t params = 0, bool action = false, bool tttext = false, bool ttformat = false, bool textformat = false, bool paint = true) {
	std::string result = parent + std::string("->add_element<") + storage_type(e) + ">(";
	result += gen_add_string_body(totalparams, e, parent, width, height, text, params, action, tttext, ttformat, textformat, paint);
	result += ");";
	return result;
}

void write_gen_function_contents(int totalparams, INOUT(uie) e, HANDLE file, std::string parent, bool store, int base_left, int tabs, bool tempstore);

void write_position_header(INOUT(uie) e, HANDLE file, int &tabs) {
	if (e.position == "same") {
		write_tb_string(file, "maxy = std::max(y, maxy);", tabs);
		write_tb_string(file, "y = tempy;", tabs);
	} else if (e.position == "indent") {
		write_tb_string(file, "basex += 15;", tabs);
		write_tb_string(file, "x = basex;", tabs);
		write_tb_string(file, "tempy = std::max(y, maxy);", tabs);
		write_tb_string(file, "y = tempy;", tabs);
	} else if (e.position == "unindent") {
		write_tb_string(file, "basex -= 15;", tabs);
		write_tb_string(file, "x = basex;", tabs);
		write_tb_string(file, "tempy = std::max(y, maxy);", tabs);
		write_tb_string(file, "y = tempy;\r\n", tabs);
	} else if (e.position == "abs") {
		write_tb_string(file, std::string("{ int x = 0; int y = 0;"), tabs);
		++tabs;
	} else if (e.position == "absx") {
		write_tb_string(file, std::string("{ int x = 0; int y = tempy;"), tabs);
		++tabs;
	} else {
		write_tb_string(file, "x = basex;", tabs);
		write_tb_string(file, "tempy = std::max(y, maxy);", tabs);
		write_tb_string(file, "y = tempy;", tabs);
	}
}

void write_position_end(INOUT(uie) e, HANDLE file, int &tabs) {
	if (e.position == "abs" || e.position == "absx") {
		--tabs;
		write_tb_string(file, std::string("}"), tabs);
	}
}

void write_update_function_contents(int totalparams, INOUT(uie) e, HANDLE file, int base_left, int tabs, bool modal_update) {
	const bool elements_rearranged = needs_update(e);
	if(e.type != "uiPanes" && e.type != "uiTabs")
		write_tb_string(file, e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "") + "->subelements.clear();", tabs);

	if (elements_rearranged) {
		write_tb_string(file, "{ int y = " DEF_SPACING "; int x = " DEF_SPACING "; int tempy = y; int maxy = y; int basex = x;", tabs);
		++tabs;
	}

	for (IN(auto) s : e.subelements) {
		if (perm_storable(*s) || (modal_update && s->type == "modal_button")) {
			if (elements_rearranged && e.type != "uiPanes" && e.type != "uiTabs") {

				write_position_header(*s, file, tabs);

				if (s->x >= 0) {
					write_tb_string(file, s->name + "->pos.left = x + " + std::to_string(s->x) + ";", tabs);
				} else {
					write_tb_string(file, s->name + "->pos.left = " + e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "") + "->pos.width + " + std::to_string(s->x) + ";", tabs);
				}
				if (s->y >= 0) {
					write_tb_string(file, s->name + "->pos.top = y + " + std::to_string(s->y) + ";", tabs);
				} else {
					write_tb_string(file, s->name + "->pos.top = " + e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "") + "->pos.height + " + std::to_string(s->y) + ";", tabs);
				}

				if (s->type == "text" || s->type == "uiCheckBox") {
					write_wh(*s, file, s->name, e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : ""), tabs);
				} else {
					if (s->width < 0) {
						if (s->x >= 0) {
							write_tb_string(file, s->name + "->pos.width = " + e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "") + "->pos.width - (x + " + std::to_string(s->x) + ") + " + std::to_string(s->width) + ";", tabs);
						} else {
							write_tb_string(file, s->name + "->pos.width = " + std::to_string(s->width) + " -  " + std::to_string(s->x) + ";", tabs);
						}
					}
					if (s->height < 0) {
						if (s->y >= 0) {
							write_tb_string(file, s->name + "->pos.height = " + e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "") + "->pos.height - (y + " + std::to_string(s->y) + ") + " + std::to_string(s->height) + ";", tabs);
						} else {
							write_tb_string(file, s->name + "->pos.height = " + std::to_string(s->height) + " -  " + std::to_string(s->y) + ";", tabs);
						}
					}
					if (s->type == "uiTabs") {
						write_tb_string(file, s->name + "->update_tab_sizes();", tabs);
					} else if (s->type == "uiPanes") {
						write_tb_string(file, s->name + "->update_size();", tabs);
					}

					write_wh(*s, file, s->name + (s->type == "window" || s->type == "modal_window" ? "_window" : ""), e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : ""), tabs);
				}

				write_position_end(*s, file, tabs);
			}

			write_update_function_contents(totalparams, *s, file, 5, tabs, modal_update);

			if (e.type != "uiPanes" && e.type != "uiTabs")
				write_tb_string(file, e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "") + "->subelements.push_back(" + s->name + ");", tabs);
	
			if (s->type == "uiDropDown" && !s->static_obj) {
				write_tb_string(file, s->name + "->reset_options();", tabs);
				write_tb_string(file, s->name + "_options(" + s->name + ", obj, " + extra_params(totalparams) + "l);", tabs);
			} else if (s->type == "ui_button_disable" || s->type == "uiCheckBox") {
				write_tb_string(file, s->name + "->enable();", tabs);
				write_tb_string(file, std::string("disable_") + s->name + "(" + s->name + ", obj, " + extra_params(totalparams) + "l);", tabs);
			} else if (s->type == "uiScrollView" || s->type == "pane" || s->type == "panel" || s->type == "tab") {
				write_tb_string(file, s->name + "->calcTotalHeight();", tabs);
			} else if (s->type == "uiHozContainer") {
				write_tb_string(file, s->name + "->RecalcPos();", tabs);
			} else if (s->type == "uiBar") {
				write_tb_string(file, s->name + "->barpos = static_cast<unsigned int>((" + s->parameters[0] + ") * " + s->name + "->pos.width);", tabs);
			} else if (s->type == "ui_toggle_button" && s->toggle.length() != 0) {
				write_tb_string(file, s->name + "->set_state(" + s->toggle + ");", tabs);
			} else if (s->type == "large_chbutton") {
				write_tb_string(file, std::string("UpdateChIcon(") + s->name = ", " + s->parameters[0] + ", 64);" , tabs);
			} else if ( s->type == "small_chbutton") {
				write_tb_string(file, std::string("UpdateChIcon(") + s->name = ", " + s->parameters[0] + ", 32);", tabs);
			} else if (s->type == "large_tbutton") {
				write_tb_string(file, std::string("UpdateTIcon(") + s->name = ", " + s->parameters[0] + ", 64);", tabs);
			} else if ( s->type == "small_tbutton") {
				write_tb_string(file, std::string("UpdateTIcon(") + s->name = ", " + s->parameters[0] + ", 32);", tabs);
			}
		} else {
			write_gen_function_contents(totalparams, *s, file, e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : ""), false, base_left, tabs, true);
		}

	}

	if (elements_rearranged) {
		--tabs;
		write_tb_string(file, "}", tabs);
	}
}

void write_gen_function_contents(int totalparams, INOUT(uie) e, HANDLE file, std::string parent, bool store, int base_left, int tabs, bool tempstore) {
	if (store && !perm_storable(e) && !(tempstore && can_store(e)))
		return;

	const bool store_element = (store || tempstore) && can_store(e);

	if(e.type != "conditional")
		write_position_header(e, file, tabs);

	if (e.type == "text") {
		if (e.parameters.size() == 0) {
			if (store_element) {
				write_tb_string(file, e.name + " = " + gen_add_string(totalparams, e, parent), tabs);
				write_wh(e, file, e.name, parent, tabs);
			} else {
				write_tb_string(file, "{", tabs);
				write_tb_string(file, std::string("const auto tx = ") + gen_add_string(totalparams, e, parent), tabs + 1);
				write_wh(e, file, "tx", parent, tabs+1);
				write_tb_string(file, "}", tabs);
			}
		} else {
			write_param_block_open(e, file, tabs);

			std::string elem = std::string("x = (get_linear_ui(TX_" + e.text + ", params, " + std::to_string(e.parameters.size()) + "," + parent + ", " + gen_add_string_body(totalparams, e, parent, false, false, false, 0, false, false, false, false, false) + ", global::" + e.paint[0] + ", global::" + e.textformat + ") + " DEF_SPACING + (e.x > 0 ? std::string(" + ") + std::to_string(e.x) : std::string("")) + ");");
			write_tb_string(file, elem, tabs);
			write_tb_string(file, std::string("y += (global::") + e.textformat + ".csize + " DEF_SPACING + (e.y > 0 ? std::string(" + ") + std::to_string(e.y) : std::string("")) + ");", tabs);

			write_param_block_close(e, file, tabs);
		}
		
	} else if (e.type == "list") {
		if (e.x > 0)
			write_tb_string(file, std::string("x += ") + std::to_string(e.x) + ";", tabs);
		else if (e.x < 0)
			write_tb_string(file, std::string("x = ") + parent + "->pos.width + " + std::to_string(e.x) + ";", tabs);

		if (e.y > 0)
			write_tb_string(file, std::string("y += ") + std::to_string(e.y) + ";", tabs);
		else if (e.y < 0)
			write_tb_string(file, std::string("y = ") + parent + "->pos.height + " + std::to_string(e.y) + ";", tabs);

		if (e.position == "same") {
			write_tb_string(file, "{ int btempy = y;", tabs);
		} else {
			write_tb_string(file, "{ int btempx = x;", tabs);
		}

		if (e.contentlist.length() != 0) {
			write_tb_string(file, std::string("for(auto it = std::begin(") + e.contentlist + "); it != std::end(" + e.contentlist + "); ++it) {", tabs + 1);
		} else if(e.start.length() > 0) {
			if (e.contentvector.length() != 0) {
				write_tb_string(file, std::string("std::vector<std::remove_cv<decltype(param_four_type(&") + e.contentfunction + "))>::type>  vec;", tabs + 1);
				write_tb_string(file, e.contentvector + ";", tabs + 1);
			}
			write_tb_string(file, std::string("for(auto it = ") + e.start + "; it != " + e.end + "; ++it) {", tabs + 1);
		} else if (e.contentvector.length() != 0) {
			//write_tb_string(file, std::string("std::vector<") + e.object + ">  vec;", tabs+1);
			write_tb_string(file, std::string("std::vector<std::remove_cv<decltype(param_four_type(&") + e.contentfunction + "))>::type>  vec;", tabs + 1);
			write_tb_string(file, e.contentvector + ";", tabs + 1);
			write_tb_string(file, "for(auto it = vec.begin(); it != vec.end(); ++it) {", tabs + 1);
		}

		if (e.position == "same")
			write_tb_string(file, "y = btempy;", tabs + 2);
		else
			write_tb_string(file, "x = btempx;", tabs + 2);

		std::string pstring;
		for (IN(auto) s : e.parameters) {
			pstring += ", ";
			pstring += s;
		}
		write_tb_string(file, e.contentfunction + "(" + parent + ", x, y, *it" + pstring + ", l);", tabs + 2);

		if (e.position == "same")
			write_tb_string(file, std::string("x += " DEF_SPACING) + (e.x > 0 ? std::string(" + ") + std::to_string(e.x) : std::string("")) + ";", tabs + 2);
		else
			write_tb_string(file, std::string("y += " DEF_SPACING) + (e.y > 0 ? std::string(" + ") + std::to_string(e.y) : std::string("")) + ";", tabs + 2);

		write_tb_string(file, "}", tabs + 1);
		write_tb_string(file, "}", tabs);
	} else if (e.type == "function") {
		if(e.x > 0)
			write_tb_string(file, std::string("x += ") + std::to_string(e.x) + ";", tabs);
		else if(e.x < 0)
			write_tb_string(file, std::string("x = ") + parent + "->pos.width + " + std::to_string(e.x) + ";", tabs);

		if(e.y > 0)
			write_tb_string(file, std::string("y += ") + std::to_string(e.y) + ";", tabs);
		else if (e.y < 0)
			write_tb_string(file, std::string("y = ") + parent + "->pos.height + " + std::to_string(e.y) + ";", tabs);

		std::string pstring = e.object.size() != 0 ? (std::string(", ") + e.object[0]) : std::string("");
		for (IN(auto) s : e.parameters) {
			pstring += ", ";
			pstring += s;
		}
		write_tb_string(file, e.contentfunction + "(" + parent + ", x, y" + pstring + ", l);", tabs + 2);
	} else if (e.type == "conditional") {
		if (!store) {
			write_tb_string(file, std::string("if(") + e.contentfunction + ") {", tabs);
			++tabs;
			for (IN(auto) s : e.subelements) {
				write_gen_function_contents(totalparams, *s, file, parent, false, 5, tabs, tempstore);
			}
			--tabs;
			write_tb_string(file, "}", tabs);
		}
	} else if (e.type == "uiCountDown") {
		if (e.parameters.size() > 2) {
			write_param_block_open(e, file, tabs, 2);
			std::string creation = ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + parent + std::string("->add_element<") + e.type + ">(" + gen_add_string_body(totalparams, e,parent,true,true,false, 0, false, false, false, false, false) + ", " +
				e.parameters[0] + ", " + e.parameters[1] + ", global::" + e.paint[0] + ", get_p_string(TX_" + e.text + ", params, " + std::to_string(e.parameters.size() - 2) + "), global::" + e.textformat + ", " + e.name + "_action" + (e.static_obj ? "" : "(obj, l)") + ");";
			write_tb_string(file, creation, tabs);
			write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
			write_param_block_close(e, file, tabs, 2);
		} else {
			write_param_block_open(e, file, tabs, 2);
			std::string creation = ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + parent + std::string("->add_element<") + e.type + ">(" + gen_add_string_body(totalparams, e, parent, true, true, false) + ", " +
				e.parameters[0] + ", " + e.parameters[1] + ", global::" + e.paint[0] + ", get_simple_string(TX_" + e.text + "),  global::" + e.textformat + ", " + e.name + "_action" + (e.static_obj ? "" : "(obj, l)") + ");";
			write_tb_string(file, creation, tabs);
			write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
			write_param_block_close(e, file, tabs, 2);
		}
	} else if (e.type == "uiButton" || e.type == "modal_button") {
		write_param_block_open(e, file, tabs);
		write_tb_string(file, ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + gen_add_string(totalparams, e, parent, true, true, true, e.parameters.size(), true, false, false), tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		write_param_block_close(e, file, tabs);
	} else if (e.type == "large_chbutton") {
		write_tb_string(file, "{", tabs);
		++tabs;
		write_tb_string(file, ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + "generateButton(" + e.parameters[0] + ", " + parent + ", " + gen_add_string_body(0, e, parent, false, false, false, 0, false, false, false, false, false) +  ", true);", tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		--tabs;
		write_tb_string(file, "}", tabs);
	} else if (e.type == "small_chbutton") {
		write_tb_string(file, "{", tabs);
		++tabs;
		write_tb_string(file, ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + "generateButton(" + e.parameters[0] + ", " + parent + ", " + gen_add_string_body(0, e, parent, false, false, false, 0, false, false, false, false, false) + ", false);", tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		--tabs;
		write_tb_string(file, "}", tabs);
	} else if (e.type == "large_tbutton") {
		write_tb_string(file, "{", tabs);
		++tabs;
		write_tb_string(file, ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + "generateTButton(" + e.parameters[0] + ", " + parent + ", " + gen_add_string_body(0, e, parent, false, false, false, 0, false, false, false, false, false) + ", true);", tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		--tabs;
		write_tb_string(file, "}", tabs);
	} else if (e.type == "small_tbutton") {
		write_tb_string(file, "{", tabs);
		++tabs;
		write_tb_string(file, ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + "generateTButton(" + e.parameters[0] + ", " + parent + ", " + gen_add_string_body(0, e, parent, false, false, false, 0, false, false, false, false, false) + ", false);", tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		--tabs;
		write_tb_string(file, "}", tabs);
	} else if (e.type == "uiBar") {
		write_param_block_open(e, file, tabs, 1);
		write_tb_string(file, (store_element ? (e.name + " = ") : std::string("const auto e = ")) + gen_add_string(totalparams, e, parent, true, true, false, e.parameters.size() != 0 ? e.parameters.size() - 1 : 0, true, false, false), tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		if (!store) {
			write_tb_string(file, (store_element ? e.name : std::string("e")) + "->barpos = static_cast<unsigned int>((" + e.parameters[0] + ") * " + (store_element ? e.name : std::string("e")) + "->pos.width);", tabs);
		}
		write_param_block_close(e, file, tabs);
	} else if (e.type == "uiGButton") {
		write_param_block_open(e, file, tabs);
		write_tb_string(file, (store_element ? (e.name + " = ") : std::string("const auto e = ")) + gen_add_string(totalparams, e, parent, true, true, false, e.parameters.size(), true, true, true), tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		write_param_block_close(e, file, tabs);
	} else if (e.type == "ui_button_disable") {
		write_param_block_open(e, file, tabs);
		write_tb_string(file, ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + gen_add_string(totalparams, e, parent, true, true, true, e.parameters.size(), true, true, true, true), tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		if (!store) {
			write_tb_string(file, std::string("disable_") + e.name + "(" + ((store_element) ? e.name : std::string("e")) + ", obj, " + extra_params(totalparams) + "l);", tabs);
		}
		write_param_block_close(e, file, tabs);
	} else if (e.type == "ui_toggle_button") {
		write_param_block_open(e, file, tabs);
		write_tb_string(file, ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) + gen_add_string(totalparams, e, parent, true, true, false, e.parameters.size(), true, true, true, false), tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		if (!store && e.toggle.length() != 0) {
			write_tb_string(file, ((store_element) ? e.name : std::string("e")) + "->set_state(" + e.toggle + ");", tabs);
		}
		write_param_block_close(e, file, tabs);
	} else if (e.type == "character_selection") {
		write_param_block_open(e, file, tabs);
		write_tb_string(file, ((store_element) ? e.name : std::string("const auto e")) + " = character_selection_menu(" + parent + ", " + gen_add_string_body(totalparams, e, parent, true, true, true, e.parameters.size(), false, false, false) + ", " + e.name + "_list" + (e.static_obj ? "" : "(obj)") + ", " + e.name + "_action" + (e.static_obj ? "" : "(obj)") + ");", tabs);
		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);
		write_param_block_close(e, file, tabs);
	} else if (e.type == "uiCheckBox") {
		write_param_block_open(e, file, tabs, 1);

		write_tb_string(file, ((store_element) ? (e.name + " = ") : std::string("const auto e = ")) +
			parent + "->add_element<uiCheckBox>(" +
			gen_add_string_body(totalparams, e, parent, false, false, true, e.parameters.size() != 0 ? e.parameters.size() - 1 : 0, false, true, true, true, false) +
			", " + (e.parameters.size() > 0 ? e.parameters[0] : std::string("false")) + ", " + 
			(e.static_obj ? e.name + "_action"  : e.name + "_action(obj, " + extra_params(totalparams) + "l)") +
			");", tabs);

		write_wh(e, file, ((store_element) ? e.name : std::string("e")), parent, tabs);

		if (!store) {
			write_tb_string(file, std::string("disable_") + e.name + "(" + ((store_element) ? e.name : std::string("e")) + ", obj, " + extra_params(totalparams) + "l);", tabs);
		}

		write_param_block_close(e, file, tabs);
	} else if (e.type == "uiDropDown") {
		write_param_block_open(e, file, tabs);
		if ((store_element)) {
			write_tb_string(file, e.name + " = " + gen_add_string(totalparams, e, parent, true, true, false, e.parameters.size(), false, false, false, true), tabs);
			if(e.static_obj)
				write_tb_string(file, e.name + "_options(" + e.name + ");", tabs);
			write_wh(e, file, e.name, parent, tabs);
		} else {
			write_tb_string(file, std::string("const auto dd = ") + gen_add_string(totalparams, e, parent, true, true, false, e.parameters.size(), false, false, false, true), tabs);
			if (!e.static_obj)
				write_tb_string(file, e.name + "_options(dd, obj, " + extra_params(totalparams) + "l);", tabs);
			else
				write_tb_string(file, e.name + "_options(dd);", tabs);
			write_wh(e, file, "dd", parent, tabs);
		}
		write_param_block_close(e, file, tabs);
	} else if (e.type == "uiHozContainer" || e.type == "uiScrollView" || e.type == "window" || e.type == "modal_window") {
		std::string container = (store_element) ? (std::string("{ ") + e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "") + " = ") : std::string("{ const auto container = ");
		if (e.type == "uiScrollView") {
			container += gen_add_string(totalparams, e, parent, true, true, false, 0, false, false, false, false, false);
		} else if(e.type == "window" || e.type == "modal_window") {
			container += gen_add_string(totalparams, e, parent, true, true, false);
		} else if (e.type == "uiHozContainer") {
			container += gen_add_string(totalparams, e, parent, true, true, true);
		}
		write_tb_string(file, container, tabs);

		write_tb_string(file, "{ int y = " DEF_SPACING "; int x = " DEF_SPACING "; int tempy = y; int maxy = y; int basex = x;", tabs + 1);

		for (IN(auto) s : e.subelements) {
			write_gen_function_contents(totalparams, *s, file, (store_element) ? (e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "")) : std::string("container"), store, 5, tabs + 2, tempstore);
		}

		if (e.type == "uiScrollView") {
			write_tb_string(file, ((store_element) ? e.name : std::string("container")) + "->calcTotalHeight();", tabs + 2);
		} else if (e.type == "uiHozContainer") {
			write_tb_string(file, ((store_element) ? e.name : std::string("container")) + "->RecalcPos();", tabs + 2);
		} else if (store && (e.type == "window" || e.type == "modal_window")) {
			write_tb_string(file, e.name + "_window->setVisible(false);", tabs + 2);
		}
		
		write_tb_string(file, "}", tabs + 1);

		write_wh(e, file, ((store_element) ? (e.name + (e.type == "window" || e.type == "modal_window" ? "_window" : "")) : std::string("container")), parent, tabs);

		write_tb_string(file, "}", tabs);
	} else if (e.type == "uiTabs" || e.type == "uiPanes") {
		if(e.type == "uiTabs")
			write_tb_string(file, ((store_element) ? (std::string("{ ") + e.name + " = ") : std::string("{ const auto container = ")) + gen_add_string(totalparams, e, parent, true, true, false, 0, false, false, false, false, false), tabs);
		else if (e.type == "uiPanes")
			write_tb_string(file, ((store_element) ? (std::string("{ ") + e.name + " = ") : std::string("{ const auto container = ")) + gen_add_string(totalparams, e, parent, true, true, false, 0, false, false, false, false, false), tabs);
		std::vector<std::string> tabnames;
		for (IN(auto) s : e.subelements) {
			if (s->type == "pane" || s->type == "tab" || s->type.length() == 0) {
				tabnames.push_back(s->text);
			}
		}
		if (e.type == "uiTabs") {
			std::string tvector("const std::vector<std::wstring> tabstxt = {");
			if (tabnames.size() != 0) {
				for (size_t i = 0; i != tabnames.size() - 1; ++i) {
					tvector += "get_simple_string(TX_";
					tvector += tabnames[i];
					tvector += "), ";
				}
				tvector += "get_simple_string(TX_";
				tvector += tabnames[tabnames.size()-1];
				tvector += ")";
			}
			tvector += "};";
			write_tb_string(file, tvector, tabs + 1);
			write_tb_string(file, ((store_element) ? e.name : std::string("container")) + std::string("->init(global::") + e.textformat + ", global::" + e.paint[0] + ", tabstxt);", tabs + 1);
		} else {
			write_tb_string(file, ((store_element) ? e.name : std::string("container")) + std::string("->init(") + std::to_string(tabnames.size()) + ");", tabs + 1);
		}

		size_t cnt = 0;
		for (IN(auto) s : e.subelements) {
			if (s->type == "pane" || s->type == "tab" || s->type.length() == 0) {
				write_tb_string(file, "{ int y = " DEF_SPACING "; int x = " DEF_SPACING "; int tempy = y; int maxy = y; int basex = x;", tabs + 1);

				if ((store_element)) {
					write_tb_string(file, s->name + " = " + e.name + "->panes[" + std::to_string(cnt) + "];", tabs + 2);
				}

				std::string pstring = ((store_element) ? e.name : std::string("container")) + "->panes[" + std::to_string(cnt) + "]";

				for (IN(auto) t : s->subelements) {
					write_gen_function_contents(totalparams, *t, file, pstring, store, 5, tabs + 2, tempstore);
				}

				write_tb_string(file, pstring + "->calcTotalHeight();", tabs + 2);
				++cnt;
				write_tb_string(file, "}", tabs + 1);
			}
		}

		write_wh(e, file, ((store_element) ? e.name : std::string("container")), parent, tabs);
		write_tb_string(file, "}", tabs);
	}

	if(e.type != "conditional")
		write_position_end(e, file, tabs);
}

std::string object_def(IN(std::vector<std::string>) object) {
	int cnt = 0;
	std::string result;
	for (IN(auto) s : object) {
		if (cnt == 0) {
			result += "IN(";
			result += s;
			result += ") obj, ";
		} else {
			result += "IN(";
			result += s;
			result += ") p";
			result += std::to_string(cnt);
			result += ", ";
		}
		++cnt;
	}
	return result;
}

bool write_action_functions(HANDLE file, IN(std::vector<std::unique_ptr<uie>>) v, IN(std::vector<std::string>) object) {
	bool added = false;
	for (IN(auto) e : v) {
		if(e->type == "modal_button") {
			write_tb_string(file, std::string("std::function<void(") + storage_type(*e) + "*)> " + e->name + "_action(" + object_def(object) + "INOUT(event) signal, IN(g_lock) l);", 1);
		} else if (e->type == "uiButton" || e->type == "uiGButton" || e->type == "ui_button_disable" || e->type == "uiCheckBox" || e->type == "ui_toggle_button" || e->type == "uiCountDown") {
			if(e->static_obj)
				write_tb_string(file, std::string("void ") + e->name + "_action(" + storage_type(*e) + "* obj);", 1);
			else
				write_tb_string(file, std::string("std::function<void(") + storage_type(*e) + "*)> " + e->name + "_action(" + object_def(object) + "IN(g_lock) l);", 1);
			if (e->type == "ui_button_disable" || e->type == "uiCheckBox") {
				write_tb_string(file, std::string("void disable_") + e->name + "(IN(std::shared_ptr<" + storage_type(*e) + ">) element, " + object_def(object) + "IN(g_lock) l);", 1);
			}
			added = true;
		} else if (e->type == "uiBar") {
			if (e->static_obj)
				write_tb_string(file, std::string("void ") + e->name + "_action(" + storage_type(*e) + "* obj, double v);", 1);
			else
				write_tb_string(file, std::string("std::function<void(") + storage_type(*e) + "*, double)> " + e->name + "_action(" + object_def(object) + "IN(g_lock) l);", 1);
			added = true;
		} else if (e->type == "uiDropDown") {
			if (e->static_obj)
				write_tb_string(file, std::string("void ") + e->name + "_options(IN(std::shared_ptr<uiDropDown>) dd);", 1);
			else
				write_tb_string(file, std::string("void ") + e->name + "_options(IN(std::shared_ptr<uiDropDown>) dd, " + object_def(object) + "IN(g_lock) l);", 1);
			added = true;
		} else if (e->type == "character_selection") {
			if (e->static_obj){
				write_tb_string(file, std::string("void ") + e->name + "_list(INOUT(cvector<char_id_t>) vec);", 1);
				write_tb_string(file, std::string("void ") + e->name + "_action(char_id_t id);", 1);
			} else {
				auto os = object_def(object);
				os = os.substr(0, os.length() - 2);
				write_tb_string(file, std::string("std::function<void(cvector<char_id_t>&)> ") + e->name + "_list(" + os + ");", 1);
				write_tb_string(file, std::string("std::function<void(char_id_t)> ") + e->name + "_action(" + os + ");", 1);
			}
			added = true;
		}
		added = write_action_functions(file, e->subelements, object) || added;
	}
	return added;
}

void write_uie_header(IN(uie) e, HANDLE file) {
	write_tb_string(file, std::string("namespace ") + e.name + " {", 0);
	if(write_action_functions(file, e.subelements, e.object))
		write_tb_string(file, "", 0);
	
	
	if (e.type == "window") {
		write_uiptrs(e, file, true, 1, "");
		write_tb_string(file, "extern std::shared_ptr<uiGButton> close_button;", 2);
		write_tb_string(file, "", 0);
		write_tb_string(file, "void init();", 1);
		write_tb_string(file, std::string("void update(") + object_def(e.object) + "IN(g_lock) l);", 1);
	} else if (e.type == "modal_window") {
		write_uiptrs(e, file, true, 1, "");
		write_tb_string(file, "", 0);
		write_tb_string(file, "void init();", 1);
		const auto od = object_def(e.object);
		write_tb_string(file, std::string("void open(") + od.substr(0, od.length() - 2) + ");", 1);
		write_tb_string(file, std::string("void update(") + object_def(e.object) + "IN(g_lock) l);", 1);
	} else {
		write_tb_string(file, std::string("void generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, ") + object_def(e.object) + "IN(g_lock) l);", 1);
	}
	write_tb_string(file, "}", 0);
}

void write_uie_body(INOUT(uie) e, HANDLE file) {
	//write_tb_string(file, std::string("namespace ") + e.name + " {", 0);
	
	if (e.type == "window") {
		write_uiptrs(e, file, false, 0, e.name);
		write_tb_string(file, std::string("std::shared_ptr<uiGButton> ") + e.name + "::close_button;", 1);
		write_tb_string(file, "", 0);

		write_tb_string(file, std::string("void ") + e.name + "::init() {", 0);
		write_tb_string(file, "int y = " DEF_SPACING "; int x = " DEF_SPACING "; int tempy = y; int maxy = y; int basex = x;", 1);
		write_gen_function_contents(static_cast<int>(e.object.size()), e, file, "global::uicontainer", true, 5, 1, false);
		write_tb_string(file, std::string("close_button = ") + e.name + "_window->add_element<uiGButton>(" + std::to_string(e.width) + " - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { " + e.name + "::" + e.name + "_window->setVisible(false); });", 1);
		write_tb_string(file, "}", 0);
		write_tb_string(file, "", 0);

		write_tb_string(file, std::string("void ") + e.name + "::update(" + object_def(e.object) + "IN(g_lock) l) {", 0);

		write_update_function_contents(static_cast<int>(e.object.size()), e, file, 5, 1, false);
		if (needs_update(e))
			write_tb_string(file, e.name + "_window->subelements.push_back(close_button);", 1);

		
		write_tb_string(file, "}", 0);
	} else if (e.type == "modal_window") {
		write_uiptrs(e, file, false, 0, e.name);
		write_tb_string(file, "", 0);

		write_tb_string(file, std::string("void ") + e.name + "::init() {", 0);
		write_tb_string(file, "int y = " DEF_SPACING "; int x = " DEF_SPACING "; int tempy = y; int maxy = y; int basex = x;", 1);
		write_gen_function_contents(static_cast<int>(e.object.size()), e, file, "global::uicontainer", true, 5, 1, false);
		write_tb_string(file, "}", 0);
		write_tb_string(file, "", 0);

		const auto od = object_def(e.object);
		write_tb_string(file, std::string("void ") + e.name + "::open(" + od.substr(0, od.length()-2) + ") {", 0);

		write_tb_string(file, "event signal;", 1);
		write_tb_string(file, "{ r_lock l;", 1);
		write_update_function_contents(static_cast<int>(e.object.size()), e, file, 5, 1, false);
		write_tb_string(file, "}", 1);

		write_tb_string(file, "", 1);
		write_tb_string(file, std::string("open_window_centered(") + e.name + "_window);", 1);
		write_tb_string(file, "event* earray[] = {&signal, &global::quitevent};", 1);
		write_tb_string(file, "event::wait_for_multiple(earray, 2, false);", 1);
		write_tb_string(file, e.name + "_window->setVisible(false);", 1);
		write_tb_string(file, "}", 0);

		write_tb_string(file, std::string("void ") + e.name + "::update(" + object_def(e.object) + "IN(g_lock) l) {", 0);
		write_update_function_contents(static_cast<int>(e.object.size()), e, file, 5, 1, true);
		write_tb_string(file, "}", 0);
	} else {
		write_tb_string(file, std::string("void ") + e.name + "::generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, " + object_def(e.object) + "IN(g_lock) l) {", 0);
		write_tb_string(file, "int x = ix; int y = iy; int tempy = iy; int maxy = iy; int basex = ix;", 1);
		for (IN(auto) s : e.subelements) {
			write_gen_function_contents(static_cast<int>(e.object.size()), *s, file, "parent", false, 5, 1, false);
			write_tb_string(file, "ix = std::max(x, ix);", 1);
			write_tb_string(file, "iy = std::max(y, iy);", 1);
		}
		write_tb_string(file, "}", 0);
	}
	//write_tb_string(file, "}", 0);
}

int __cdecl wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
	std::vector<std::string> includes;
	

	if (argc > 1) {
		std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
		ParseCKFile(argv[1], results);
		for (IN(auto) p : results->list) {
			if (p.list) {
				top_lvl_containers.emplace_back(std::move(parse_ui_element(p.list.get())));
			} else if (p.assoc) {
				if (p.assoc->left == "include")
					includes.push_back(p.assoc->right);
				else if (p.assoc->left == "class")
					extras.push_back(std::string("class ") + p.assoc->right + ";");
				else if (p.assoc->left == "struct")
					extras.push_back(std::string("struct ") + p.assoc->right + ";");
			}
		}
		
		size_t count = 1;
		auto_name(top_lvl_containers, count);
		fix_params(top_lvl_containers);
		default_values(top_lvl_containers);

		std::wstring fnamebase(argv[1]);
		fnamebase = fnamebase.substr(0, fnamebase.length() - 3);

		HANDLE hfile =  CreateFile((fnamebase + L"h").c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, NULL);
		write_tb_string(hfile, "#pragma once", 0);
		write_tb_string(hfile, "#include \"globalhelpers.h\"", 0);
		write_tb_string(hfile, "#include \"uielements.hpp\"", 0);
		write_tb_string(hfile, "", 0);

		for (IN(auto) s : extras) {
			write_tb_string(hfile, s, 0);
		}

		write_tb_string(hfile, "", 0);
		write_tb_string(hfile, "void init_all_generated_ui();", 0);
		write_tb_string(hfile, "", 0);


		for (IN(auto) e : top_lvl_containers) {
			write_uie_header(*e, hfile);
			write_tb_string(hfile, "", 0);
		}
		CloseHandle(hfile);

		hfile = CreateFile((fnamebase + L"cpp").c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, NULL);
		write_tb_string(hfile, "#include \"globalhelpers.h\"", 0);
		write_tb_string(hfile, std::string("#include \"") + wstr_to_str(fnamebase) +"h\"", 0);
		write_tb_string(hfile, "#include \"i18n.h\"", 0);
		write_tb_string(hfile, "#include \"structs.hpp\"", 0);
		for (IN(auto) st : includes) {
			write_tb_string(hfile, std::string("#include \"") + st + ".h\"", 0);
		}
		write_tb_string(hfile, "", 0);

		write_tb_string(hfile, "#pragma  warning(push)", 0);
		write_tb_string(hfile, "#pragma  warning(disable:4456)", 0);

		write_tb_string(hfile, "", 0);

		write_tb_string(hfile, "template <typename A, typename B, typename C, typename T, typename ... REST>", 0);
		write_tb_string(hfile, "T param_four_type(void(*f_ptr)(A, B, C, T, REST ... rest)) { return T(); }", 0);
		write_tb_string(hfile, "", 0);
		write_tb_string(hfile, "template <typename A, typename B, typename C, typename T, typename ... REST>", 0);
		write_tb_string(hfile, "T param_four_type(void(*f_ptr)(A, B, C, T&, REST ... rest)) { return T(); }", 0);
		write_tb_string(hfile, "", 0);
		
		write_tb_string(hfile, "void init_all_generated_ui() {", 0);
		for (IN(auto) e : top_lvl_containers) {
			if (e->type == "window" || e->type == "modal_window") {
				write_tb_string(hfile, e->name + "::init();", 1);
			}
		}
		write_tb_string(hfile, "}", 0);
		write_tb_string(hfile, "", 0);

		for (IN(auto) e : top_lvl_containers) {
			write_uie_body(*e, hfile);
			write_tb_string(hfile, "", 0);
		}

		write_tb_string(hfile, "#pragma  warning(pop)", 0);

		CloseHandle(hfile);
	} else {
		OutputDebugString(TEXT("No arguments provided"));
	}
}
