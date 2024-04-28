import json
import matplotlib.pyplot as plt
import networkx as nx

# Load JSON data
file_path = '/home/cspl/Shore-user/case-study/catkin_ws/src/orb_slam3_ros_wrapper/haros_result/results/data/ORB_SLAM3/configurations.json'  # Use the correct path
with open(file_path, 'r') as file:
    data = json.load(file)

# Select a configuration to visualize
config_name = 'euroc_stereoimu'
config = next((item for item in data if item['id'] == config_name), None)

# Initialize directed graph
G = nx.DiGraph()

# Helper function to format label with component details
def format_label(details):
    label = ""
    if details:
        file_info = details.get('file', 'N/A')
        line_info = details.get('line', 'N/A')
        column_info = details.get('column', 'N/A')
        label = f" ({file_info} L:{line_info} C:{column_info})"
    return label


# Add nodes with their labels
for node in config['nodes']:
    G.add_node(node['name'], type='node', label=node['name'])

# Add topics with their labels and additional details
for topic in config['topics']:
    detail = topic.get('traceability', [{}])[0]  # Taking first traceability entry if exists
    additional_info = format_label(detail)
    G.add_node(topic['name'], type='topic', label=topic['name'], detail=additional_info)

if 'threads' in config:
    for threads in config['threads']:
        label = ""
        if threads:
            file_info = threads.get('file', 'N/A')
            line_info = threads.get('line', 'N/A')
            column_info = threads.get('column', 'N/A')
            label = f" ({file_info} L:{line_info} C:{column_info})"
        G.add_node(threads['name'], type='threads', label=threads['name'], detail=label)

# Add edges for publishers and subscribers
for link in config['links']['publishers']:
    G.add_edge(link['node'], link['topic'], type='publishes')

for link in config['links']['subscribers']:
    G.add_edge(link['topic'], link['node'], type='subscribes')

if 'threads' in config:    
    for link in config['threads']:
        G.add_edge(link['node'], link['name'], type='threads')

# for node in G:
#     print(G.nodes[node])
# Position nodes using the spring layout
pos = nx.spring_layout(G, k=20, iterations=500)
# pos = nx.shell_layout(G)

plt.figure(figsize=(9, 5))

# Draw nodes and topics differently
node_colors = ['white' if G.nodes[node]['type'] == 'node' else 'lightgreen' if G.nodes[node]['type'] == 'topic' else 'yellow' for node in G]
nx.draw_networkx_nodes(G, pos, node_color=node_colors, edgecolors='black', node_size=2000)
nx.draw_networkx_edges(G, pos, edge_color='gray')

# Draw labels for nodes and topics
node_labels = {node: G.nodes[node]['label'] for node in G if G.nodes[node]['type'] == 'node'}
topic_labels = {node: G.nodes[node]['label'] for node in G if G.nodes[node]['type'] == 'topic'}
threads_labels = {node: G.nodes[node]['label'] for node in G if G.nodes[node]['type'] == 'threads'}
nx.draw_networkx_labels(G, pos, labels=node_labels, font_color='black', font_weight='bold', clip_on=False)
nx.draw_networkx_labels(G, pos, labels=topic_labels, font_weight='bold', clip_on=False)
if threads_labels:
    nx.draw_networkx_labels(G, pos, labels=threads_labels, font_weight='bold', clip_on=False)

# Draw edges with arrows
nx.draw_networkx_edges(G, pos, edgelist=G.edges(), edge_color='gray', arrowsize=20, node_size=2000)

# Draw additional detail beside each topic
for topic, details in topic_labels.items():
    if 'detail' in G.nodes[topic]:
        detail_text = G.nodes[topic]['detail']
        x, y = pos[topic]
        plt.text(x - 0.5, y + 0.2, detail_text, fontsize=10, ha='left', va='center', wrap=True)

if threads_labels:
    for threads, details in threads_labels.items():
        if 'detail' in G.nodes[threads]:
            detail_text = G.nodes[threads]['detail']
            x, y = pos[threads]
            plt.text(x - 0.5, y + 0.1, detail_text, fontsize=10, ha='left', va='center', wrap=True)

plt.axis('off')
# plt.title(f"ROS Nodes and Topics Relationship for '{config_name}' Configuration")
plt.tight_layout()
plt.savefig('haros_vis_new.pdf', format='pdf', bbox_inches='tight')