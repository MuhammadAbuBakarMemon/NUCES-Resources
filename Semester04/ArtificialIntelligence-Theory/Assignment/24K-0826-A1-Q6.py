def find_positions(maze):
    start = None
    goal = None
    rows = len(maze)
    cols = len(maze[0])
    
    for row in range(rows):
        for column in range(cols):
            if maze[row][column] == 'S':
                start = (row, column)
            elif maze[row][column] == 'G':
                goal = (row, column)
    return start, goal

def get_neighbors(node, maze):
    row, column = node
    neighbors = []
    moves = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    
    rows = len(maze)
    cols = len(maze[0])
    
    for dr, dc in moves:
        new_row = row + dr
        new_column = column + dc
        
        if new_row >= 0 and new_row < rows and new_column >= 0 and new_column < cols:
            if maze[new_row][new_column] != 1:
                neighbors.append((new_row, new_column))
                
    return neighbors

def depth_limited_search(maze, start, goal, limit):
    stack = []
    stack.append((start, [start]))
    
    print(f"Testing Limit: {limit}")
    
    while len(stack) > 0:
        current_node, path = stack.pop()

        if current_node == goal:
            return path
        
        if len(path) <= limit:
            neighbors = get_neighbors(current_node, maze)
            
            for neighbor in neighbors:
                if neighbor not in path:
                    new_path = list(path)
                    new_path.append(neighbor)
                    stack.append((neighbor, new_path))
                    
    return None

def iterative_deepening_search(maze, start, goal):
    depth_limit = 0
    
    while True:
        result = depth_limited_search(maze, start, goal, depth_limit)
        
        if result is not None:
            print(f"Goal found at depth limit: {depth_limit}")
            return result
        
        print(f"Goal not found at depth {depth_limit}. Increasing depth.")
        depth_limit = depth_limit + 1
        
        if depth_limit > 50:
            print("Goal unreachable or depth too high.")
            return None

maze = [
    ['S', 0, 0, 1, 0],
    [1, 0, 1, 0, 0],
    [0, 0, 0, 0, 'G'],
    [1, 1, 0, 1, 1]
]

start_pos, goal_pos = find_positions(maze)

print("Starting Iterative Deepening Search:")
print(f"Start: {start_pos}, Goal: {goal_pos}")

final_path = iterative_deepening_search(maze, start_pos, goal_pos)

if final_path:
    print("FINAL PATH FOUND:", final_path)
else:
    print("Path not found.")