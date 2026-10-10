"""
Unit tests for SatisTools Map algorithms:
- World coordinate transformations
- Point-in-polygon (Lasso selection)
- Zone production analytics & net throughput calculations
"""

import math
import unittest

WORLD_MIN_X = -324698.0
WORLD_MAX_X = 425301.0
WORLD_MIN_Y = -375000.0
WORLD_MAX_Y = 375000.0

def world_to_normalized_uv(x, y):
    range_x = WORLD_MAX_X - WORLD_MIN_X
    range_y = WORLD_MAX_Y - WORLD_MIN_Y
    u = max(0.0, min(1.0, (x - WORLD_MIN_X) / range_x))
    v = max(0.0, min(1.0, (y - WORLD_MIN_Y) / range_y))
    return u, v

def normalized_uv_to_world(u, v):
    range_x = WORLD_MAX_X - WORLD_MIN_X
    range_y = WORLD_MAX_Y - WORLD_MIN_Y
    x = WORLD_MIN_X + (u * range_x)
    y = WORLD_MIN_Y + (v * range_y)
    return x, y

def is_point_inside_polygon(pt, polygon):
    if len(polygon) < 3:
        return False
    inside = False
    j = len(polygon) - 1
    for i in range(len(polygon)):
        vi = polygon[i]
        vj = polygon[j]
        if ((vi[1] > pt[1]) != (vj[1] > pt[1])) and \
           (pt[0] < (vj[0] - vi[0]) * (pt[1] - vi[1]) / (vj[1] - vi[1]) + vi[0]):
            inside = not inside
        j = i
    return inside

def calculate_zone_analytics(buildings, polygon):
    analytics = {
        "total_buildings": 0,
        "total_power_mw": 0.0,
        "efficiency_percent": 0.0,
        "production": {},
        "consumption": {},
        "net": {}
    }
    
    if len(polygon) < 3:
        return analytics
        
    efficiency_sum = 0.0
    for b in buildings:
        if not is_point_inside_polygon((b["x"], b["y"]), polygon):
            continue
            
        analytics["total_buildings"] += 1
        analytics["total_power_mw"] += b.get("power_mw", 0.0)
        efficiency_sum += 100.0 if b.get("producing", False) else 0.0
        
        for item, rate in b.get("inputs", {}).items():
            analytics["consumption"][item] = analytics["consumption"].get(item, 0.0) + rate
            
        for item, rate in b.get("outputs", {}).items():
            analytics["production"][item] = analytics["production"].get(item, 0.0) + rate
            
    if analytics["total_buildings"] > 0:
        analytics["efficiency_percent"] = efficiency_sum / analytics["total_buildings"]
        
    all_items = set(analytics["production"].keys()) | set(analytics["consumption"].keys())
    for item in all_items:
        prod = analytics["production"].get(item, 0.0)
        cons = analytics["consumption"].get(item, 0.0)
        analytics["net"][item] = prod - cons
        
    return analytics

class TestSatisToolsMapLogic(unittest.TestCase):
    def test_coordinates_conversion(self):
        # Center of the world
        center_x = (WORLD_MIN_X + WORLD_MAX_X) / 2.0
        center_y = (WORLD_MIN_Y + WORLD_MAX_Y) / 2.0
        u, v = world_to_normalized_uv(center_x, center_y)
        self.assertAlmostEqual(u, 0.5, places=5)
        self.assertAlmostEqual(v, 0.5, places=5)

        # Roundtrip
        rx, ry = normalized_uv_to_world(u, v)
        self.assertAlmostEqual(rx, center_x, places=2)
        self.assertAlmostEqual(ry, center_y, places=2)

        # Clamping
        u_out, v_out = world_to_normalized_uv(-1000000.0, 1000000.0)
        self.assertEqual(u_out, 0.0)
        self.assertEqual(v_out, 1.0)

    def test_point_in_polygon(self):
        poly = [(0, 0), (100, 0), (100, 100), (0, 100)]
        self.assertTrue(is_point_inside_polygon((50, 50), poly))
        self.assertFalse(is_point_inside_polygon((150, 50), poly))
        self.assertFalse(is_point_inside_polygon((-10, 50), poly))
        # Empty polygon edge case
        self.assertFalse(is_point_inside_polygon((50, 50), []))
        self.assertFalse(is_point_inside_polygon((50, 50), [(0, 0), (10, 10)]))

    def test_analytics_aggregation(self):
        buildings = [
            {
                "x": 50, "y": 50, "power_mw": 15.0, "producing": True,
                "inputs": {"Iron Ingot": 60.0},
                "outputs": {"Iron Plate": 20.0}
            },
            {
                "x": 60, "y": 60, "power_mw": 15.0, "producing": False,
                "inputs": {"Iron Ingot": 60.0},
                "outputs": {"Iron Plate": 20.0}
            },
            {
                # Outside polygon
                "x": 500, "y": 500, "power_mw": 50.0, "producing": True,
                "inputs": {"Coal": 120.0},
                "outputs": {}
            }
        ]
        poly = [(0, 0), (100, 0), (100, 100), (0, 100)]
        res = calculate_zone_analytics(buildings, poly)
        
        self.assertEqual(res["total_buildings"], 2)
        self.assertEqual(res["total_power_mw"], 30.0)
        self.assertEqual(res["efficiency_percent"], 50.0)
        self.assertEqual(res["consumption"]["Iron Ingot"], 120.0)
        self.assertEqual(res["production"]["Iron Plate"], 40.0)
        self.assertEqual(res["net"]["Iron Ingot"], -120.0)
        self.assertEqual(res["net"]["Iron Plate"], 40.0)

if __name__ == "__main__":
    unittest.main()
