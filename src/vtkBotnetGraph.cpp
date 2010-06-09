#include "vtkBotnetGraph.h"
#include "Repeater.h"
#include "Spammer.h"

void vtkBotnetGraph::calc_points(vtkGraphLayout* layout)
{
	double pt[3];
	vtkGraph* g = vtkGraph::SafeDownCast(layout->GetOutput());
	
	vtkIdType v;
	for (v = 0; v < g->GetNumberOfVertices(); v++)
    {
		g->GetPoint(v, pt);
		printf("points %f %f %f\n",pt[0], pt[1], pt[2]);
		points->InsertNextPoint(pt[0], pt[1], pt[2]);
	}
}

void vtkBotnetGraph::update_graph()
{	
	this->repeaters = this->botnet->repeaters_list();
	this->protecters = this->botnet->protecters_list();
	this->spammers = this->botnet->spammers_list();
	this->attackers = this->botnet->attackers_list();
	
	if(this->graph_iscreate)
		delete_graph();
		
	construct_graph();
	
	this->vertex_command_and_conquer = this->graph->AddVertex();
	this->vertexcolors->InsertNextValue(1); // rouge

	update_protecters();
	update_repeaters();
	update_spammers();
	
	this->graph->Modified();
	this->graph->GetVertexData()->AddArray(this->vertexcolors);
	this->graph->GetEdgeData()->AddArray(this->edgescolors);
	
	if(this->graph_create_first_time)
	{
		printf("updategraph 5\n");
		this->graph_create_first_time = false;
		vtkGraphLayout* layout = vtkGraphLayout::New();
		vtkClustering2DLayoutStrategy* strategy = vtkClustering2DLayoutStrategy::New();
	
		layout->SetInput(this->graph);
		layout->SetLayoutStrategy(strategy);
		
		layout->Update();
		calc_points(layout);
		printf("updategraph 7\n");
	}
	
	this->graph->SetPoints(this->points);
	
    vtkGraphLayout* layout = vtkGraphLayout::New();
	//vtkCircularLayoutStrategy* strategy = vtkCircularLayoutStrategy::New();
	
	layout->SetInput(this->graph);
	layout->SetLayoutStrategy(strategy);
	
	this->graphLayoutView->AddRepresentationFromInputConnection(layout->GetOutputPort());
}

/*
void vtkBotnetGraph::update_color_repeater(boost::shared_ptr<waledac::Bot> bot)
{
	vtkIdType repeater_vertex = assoc_repeaters[repeater->id()];
	int p = (int) (waledac::Bot::percentage_compromised(repeater->rlist())*100);
	vtkIntArray::SafeDownCast(this->tree->GetVertexData()->GetArray("color"))->SetValue(repeater_vertex, p);
}
*/
void vtkBotnetGraph::update_protecters()
{
	for(unsigned int i = 0; i < this->protecters.size(); i++)
	{
		vtkIdType vertex_protecter = this->graph->AddVertex();
		/* tout les protecteurs sont liés au command and conquer */	
		this->graph->AddEdge(vertex_protecter, this->vertex_command_and_conquer);
		this->assoc_protecters[this->protecters[i]] = vertex_protecter;
      	this->vertexcolors->InsertNextValue(10); // bleu
      	this->edgescolors->InsertNextValue(10);
	}

	printf("FIN UPDATE PROTECTERS\n");
}

void vtkBotnetGraph::update_repeaters()
{
	waledac::Repeater *repeater;
	
	for(unsigned int j = 0; j < this->repeaters.size(); j++)
	{
		vtkIdType vertex_repeater =  this->graph->AddVertex();
		this->vertexcolors->InsertNextValue(4); // vert
		this->assoc_repeaters[this->repeaters[j]] = vertex_repeater;
		repeater = dynamic_cast<waledac::Repeater*>(this->repeaters[j].get());
		
		/* les répéteurs sont liés aux protécteurs */	
		for(unsigned int k = 0; k < repeater->plist().size(); k++)
		{
			this->graph->AddEdge(this->assoc_protecters[repeater->plist()[k]], vertex_repeater);
			this->edgescolors->InsertNextValue(4);
		}
	}

	// les répéteurs sont liés entre eux 
	for(unsigned int j = 0; j < this->repeaters.size(); j++)
	{
		repeater = dynamic_cast<waledac::Repeater*>(this->repeaters[j].get());
		//std::vector< boost::shared_ptr<waledac::Bot> > repeater_rlist = repeater->rlist();
	
		for(unsigned int k = 0; k < repeater->rlist().size(); k++)
		{	
			this->graph->AddEdge(this->assoc_repeaters[this->repeaters[j]], this->assoc_repeaters[repeater->rlist()[k]]);
			this->edgescolors->InsertNextValue(4);
		}
	}
	
	printf("FIN UPDATE REPEATERS\n");
}

void vtkBotnetGraph::update_spammers()
{
	for(unsigned int j = 0; j < this->spammers.size(); j++)
	{
		vtkIdType vertex_spammer =  this->graph->AddVertex();
		this->vertexcolors->InsertNextValue(2);
		this->assoc_spammers[this->spammers[j]] = vertex_spammer;
		
		waledac::Spammer *spammer = dynamic_cast<waledac::Spammer*>(this->spammers[j].get());
		
		for(unsigned int k = 0; k < spammer->rlist().size(); k++)
		{
			this->graph->AddEdge(vertex_spammer, this->assoc_repeaters[spammer->rlist()[k]]);
			this->edgescolors->InsertNextValue(2);
		}
	}
	
	printf("FIN UPDATE SPAMMERS\n")	;
}

vtkBotnetGraph::vtkBotnetGraph(unsigned int rlist_size, unsigned int plist_size, unsigned int spammers_number, unsigned int attackers_number)
{	
	this->graph_create_first_time = true;
	
	this->botnet = new waledac::Botnet(rlist_size, plist_size, spammers_number, attackers_number);
	this->botnet->init();	
	printf("BOTNET ASSIGN\n");
	
	this->interactor = new vtkBotnetInteractor;
	this->graphLayoutView = vtkGraphLayoutView::New();
	
	this->lookuptable = vtkLookupTable::New();
	this->lookuptable->SetTableRange(0.0, 10.0);
	this->lookuptable->Build();
	
	construct_graph();
	
	this->graphLayoutView->ResetCamera();
	this->graphLayoutView->Render();
	this->graphLayoutView->GetInteractor()->Start();
	
}

void vtkBotnetGraph::delete_graph()
{
	this->graph->Delete();
	this->tree->Delete();
	this->vertexcolors->Delete();
	this->edgescolors->Delete();
	this->theme->Delete();
	
	this->graph_iscreate = false;
}

void vtkBotnetGraph::construct_graph()
{
	this->vertexcolors = vtkIntArray::New();
	this->vertexcolors->SetNumberOfComponents(1);
 	this->vertexcolors->SetName("colorvertices");
 	
 	this->edgescolors = vtkIntArray::New();
	this->edgescolors->SetNumberOfComponents(1);
 	this->edgescolors->SetName("coloredges");
	
	this->tree = vtkTree::New();
	this->graph = vtkMutableUndirectedGraph::New();
	
	//this->graphLayoutView->AddRepresentationFromInput(this->graph);
	//this->graphLayoutView->SetLayoutStrategyToCone();
	
	/*****/
	this->graphLayoutView->SetVertexColorArrayName("colorvertices");
	this->graphLayoutView->SetEdgeColorArrayName("coloredges"); 
	this->graphLayoutView->ColorVerticesOn();
	this->graphLayoutView->ColorEdgesOn();
 
	this->theme = vtkViewTheme::New();
	this->theme->SetPointLookupTable(this->lookuptable);
	this->graphLayoutView->ApplyViewTheme(this->theme);
	/*****/
 
 	this->interactor->setbotnet(this);
 	this->interactor->SetDefaultRenderer(this->graphLayoutView->GetRenderer());
	this->graphLayoutView->GetInteractor()->SetInteractorStyle(this->interactor);
	
	this->graph_iscreate = true;
}

vtkBotnetGraph::~vtkBotnetGraph()
{
	//Delete();
}














