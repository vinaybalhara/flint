import { Application, Element, ApplicationState, Color } from 'Application.js'

export default class MyApplication extends Application
{
	constructor() 
	{
		super({
			size: [800, 600],
			title: 'garçon',
			state: ApplicationState.NORMAL,
			fullscreen: true
		});
	
		this.on('load', () =>
		{
			this.xx = new Element({parent: this.stage, size: [200, 200], position: [100, 100], backgroundColor: Color(255, 0, 0) });
			this.yy = new Element({parent: this.xx, size: [200, 80], position: [40, 40], backgroundColor: Color(255, 233, 0) });
			this.zz = new Element({parent: this.xx, size: [200, 80], position: [40, 140], backgroundColor: Color(55, 243, 0) });
			print('Application Loaded ' + Application.title);
		});
		
		this.on('beforeunload', () =>
		{
			print('Application Closing');
		});
		
		this.on('unload', () =>
		{
			print('Application Closed');
		});
		
		this.on('keyup', (event) =>
		{
			this.xx.release();
		});
		
		this.on('keydown', (event) =>
		{
			event.preventDefault();
		});
	}
	
}