#include "gfx/scene_renderer.h"
#include "gfx/device.h"
#include <algorithm>
#include "sys/profiler.h"

using std::make_pair;
using std::pair;

namespace gfx {

	SceneRenderer::SceneRenderer(IRect viewport, int2 view_pos) :m_viewport(viewport), m_view_pos(view_pos) {
		m_target_rect = IRect(m_view_pos, m_view_pos + m_viewport.size());
		m_elements.reserve(1024);
	}

	void SceneRenderer::add(PTexture texture, IRect rect, float3 pos, FBox bbox, Color color, FRect tex_rect) {
		DASSERT(texture);

		rect += (int2)worldToScreen(pos);
		if(!areOverlapping(rect, m_target_rect))
			return; //TODO: redundant check

		Element new_elem;
		new_elem.texture = texture;
		new_elem.bbox = bbox + pos;
		new_elem.rect = rect;
		new_elem.color = color;
		new_elem.tex_rect = tex_rect;

		m_elements.push_back(new_elem);
	}

	void SceneRenderer::addBox(FBox bbox, Color color, bool is_filled) {
		if(!is_filled) {
			m_wire_boxes.push_back(BoxElement{bbox, color});
			return;
		}

		IRect rect = (IRect)worldToScreen(bbox);
		if(!areOverlapping(rect, IRect(m_view_pos, m_view_pos + m_viewport.size())))
			return;

		Element new_elem;
		new_elem.texture = nullptr;
		new_elem.bbox = bbox;
		new_elem.rect = rect;
		new_elem.color = color;
		m_elements.push_back(new_elem);
	}

	void SceneRenderer::addLine(int3 begin, int3 end, Color color) {
		LineElement line;
		line.begin = begin;
		line.end = end;
		line.color = color;
		m_lines.push_back(line);
	}

	struct GNode {
		bool operator<(const GNode &node) const {
			return first == node.first? second < node.second : first < node.first;
		}
		int first, second, flag;
	};

	static int DFS(const vector<char> &graph, vector<GNode> &gdata, int count, int i, int time) {
		gdata[i].flag = 1;

		for(int k = 0; k < count; k++) {
			if(k == i || graph[i + k * count] != -1)
				continue;
			if(gdata[k].flag)
				profiler::updateCounter("loops", 1);
			if(gdata[k].second)
				continue;
			gdata[k].second = time++;
			time = DFS(graph, gdata, count, k, time);
		}

		gdata[i].flag = 0;
		gdata[i].first = time++;
		return time;
	}

	// TODO: Not really sure if its fast...
	int fastDrawingOrder(const FBox &a, const FBox &b) {
		bool overlap_x = b.min.x <= a.max.x && a.min.x <= b.max.x;
		bool overlap_y = b.min.y <= a.max.y && a.min.y <= b.max.y;
		bool overlap_z = b.min.z <= a.max.z && a.min.z <= b.max.z;

		int x_ret = a.max.x <= b.min.x? -1 : b.max.x <= a.min.x? 1 : 0;
		int y_ret = a.max.y <= b.min.y? -1 : b.max.y <= a.min.y? 1 : 0;
		int z_ret = a.max.z <= b.min.z? -1 : b.max.z <= a.min.z? 1 : 0;
		
		if(y_ret)
			return y_ret;

		if(x_ret)
			return x_ret;

		if(z_ret)
			return z_ret;

		return 0;
	}

	void SceneRenderer::render() {
		PROFILE("SceneRenderer::render");

		enum { node_size = 128 };

		setScissorTest(true);
		lookAt(m_view_pos - m_viewport.min);
		IRect view(m_view_pos, m_view_pos + m_viewport.size());

		int xNodes = (m_viewport.width() + node_size - 1) / node_size;
		int yNodes = (m_viewport.height() + node_size - 1) / node_size;

		std::random_shuffle(m_elements.begin(), m_elements.end());

		// Screen is divided into a set of squares. Rendered elements are assigned to covered
		// squares and sorting is done independently for each of the squares, so that, we can
		// minize problems with rendering order
		vector<std::pair<int, int> > grid;
		grid.reserve(m_elements.size() * 4);

		profiler::updateCounter("SceneRenderer::total_count", m_elements.size());
		for(int n = 0; n < (int)m_elements.size(); n++) {
			const Element &elem = m_elements[n];
			IRect rect = elem.rect - m_view_pos;

			for(int y = rect.min.y - rect.min.y % node_size; y < rect.max.y; y += node_size)
				for(int x = rect.min.x - rect.min.x % node_size; x < rect.max.x; x += node_size) {
					int grid_x = x / node_size, grid_y = y / node_size;
					if(grid_x >= 0 && grid_y >= 0 && grid_x < xNodes && grid_y < yNodes) {
						int node_id = grid_x + grid_y * xNodes;
						grid.push_back(std::make_pair(node_id, n));
					}
				}
		}

		std::sort(grid.begin(), grid.end());

		// Now we need to do topological sort for a graph in which each edge means
		// than one tile should be drawn before the other; cycles in this graph result
		// in glitches in the end (unavoidable)
		vector<char> graph(1024, 0);
		vector<GNode> gdata(32);

		for(int g = 0; g < (int)grid.size();) {
			int node_id = grid[g].first;
			int grid_x = node_id % xNodes, grid_y = node_id / xNodes;

			int count = 0;
			while(grid[g + count].first == node_id && g + count < (int)grid.size())
				count++;

			if((int)graph.size() < count * count)
				graph.resize(count * count);
			if((int)gdata.size() < count)
				gdata.resize(count);

			{ PROFILE("SceneRenderer::inner_loop"); 
				IRect rects[count];
				FBox bboxes[count];
				for(int i = 0; i < count; i++) {
					const Element &elem = m_elements[grid[g + i].second];
					rects[i] = elem.rect;
					bboxes[i] = elem.bbox;
				}
				for(int i = 0; i < count; i++) {
					bool overlaps[count];
					IRect rect = rects[i];

					for(int j = i + 1; j < count; j++) {
						int result = areOverlapping(rect, rects[j])? fastDrawingOrder(bboxes[i], bboxes[j]): 0;
						graph[i + j * count] = result;
						graph[j + i * count] = -result;
					}
				}
			}

			for(int i = 0; i < count; i++)
				gdata[i] = GNode{0, 0, 0};

			int time = 1;
			for(int i = 0; i < count; i++) {
				if(gdata[i].second)
					continue;
				gdata[i].second = time++;
				time = DFS(graph, gdata, count, i, time);
			}

			for(int i = 0; i < count; i++)
				gdata[i].second = grid[g + i].second;
			std::sort(&gdata[0], &gdata[0] + count);

			int2 grid_tl = m_viewport.min + int2(grid_x * node_size, grid_y * node_size);
			IRect grid_rect(grid_tl, grid_tl + int2(node_size, node_size));
			grid_rect.max = min(grid_rect.max, m_viewport.max);
			setScissorRect(grid_rect);

			profiler::updateCounter("SceneRenderer::rendered_count", count);
			PROFILE("SceneRenderer::blit");
			for(int i = count - 1; i >= 0; i--) {
				const Element &elem = m_elements[gdata[i].second];
				if(elem.texture) {
					elem.texture->bind();
					drawQuad(elem.rect.min, elem.rect.size(), elem.tex_rect.min, elem.tex_rect.max, elem.color);
				}
				else {
					DTexture::bind0();
					drawBBox(elem.bbox, elem.color, true);
				}
			}
			
			g += count;
		}

//		printf("\nGrid overhead: %.2f\n", (double)grid.size() / (double)m_elements.size());

		setScissorRect(m_viewport);
		DTexture::bind0();
		for(int n = 0; n < (int)m_lines.size(); n++) {
			const LineElement &line = m_lines[n];
			drawLine(line.begin, line.end, line.color);
		}
		for(int n = 0; n < (int)m_wire_boxes.size(); n++) {
			const BoxElement &elem = m_wire_boxes[n];
			drawBBox(elem.bbox, elem.color);
		}

		setScissorTest(false);
	}

}
