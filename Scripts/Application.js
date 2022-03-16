
export const ApplicationState = 
{
	NORMAL : 0,
	MINIMIZED: 1,
	MAXIMIZED: 2
};

class Event
{
	constructor(type)
	{
		this.type = type;
		this.bubbles = true;
		this.defaultPrevented = false;
	}
	
	stopPropagation() { this.bubbles = false; }
	preventDefault() { this.defaultPrevented = true; }
}

class KeyboardEvent extends Event
{
	constructor(type, keyCode, key, flags)
	{
		super(type);
		this.key = key;
		this.keyCode = keyCode;
		this.ctrlKey = !!(flags & 2);
		this.shiftKey = !!(flags & 4);
		this.altKey = !!(flags & 8);
		this.metaKey = !!(flags & 16);
		this.repeat = !!(flags & 32);
		this.location = flags >> 6;
	}
}

export class Application
{
	static __I = null;
	
	constructor(args)
	{
		if(Application.__I) 
			throw 'Class Application already has an instance.';
		Application.__I = this;
		this.processQueue = [];
		this.__spares = [];
		this.__timers = new Map();
		this.__events = {};
		this.__tid = 0;
		this.root = null;
		const result = __f_init(args);
		Application.__I.__title = result[2];
		Application.__I.__state = result[3];
	}
	
	load(_ptr)
	{
		this.stage = new Element(null, 0, _ptr); 
		if(this.onload)
			this.onload();
	}
	
	update(delta) 
	{
		if(this.onupdate)
			this.onupdate(delta);
		for (let i = 0; i !== this.processQueue.length; ++i)
		{
			const callback = this.processQueue[i];
			if(callback)
				callback(delta);
		}
		for (let [id, timer] of this.__timers) 
		{
			timer[3] += delta;
			if(timer[3] >= timer[2])
			{
				timer[1]();
				timer[3] %= timer[2];
				if(timer[0] === 1)
					this.__timers.delete(id);
			}
		}
	}
	
	dispatch(event)
	{
		const own = this['on' + event.type];
		if(own)
		{
			own(event);
			if(!event.bubbles)
				return;
		}
		const listeners = this.__events[event.type];
		if(listeners)
		{
			for (let listener of listeners)
			{
				listener(event);
				if(!event.bubbles)
					return;
			}				
		}
		return !event.defaultPrevented;
	}
	
	fireEvent(type, a, b, c)
	{
		if(type === 1)
			return this.dispatch(new KeyboardEvent('keyup', a, b, c)); 
		else if(type == 2)
			return this.dispatch(new KeyboardEvent('keydown', a, b, c)); 
		return true;
	}
	
	static addEventListener(name, callback)
	{
		if(typeof callback === 'function')
		{
			let target = Application.__I.__events[name];
			if(target === undefined)
				target = Application.__I.__events[name] = new Set();
			target.add(callback);
		}
	}
	
	static removeEventListener(name, callback)
	{
		if(typeof callback === 'function')
		{
			let target = Application.__I.__events[name];
			if(target)
				target.delete(callback);
		}
	}
	
	on(name, callback)
	{
		this['on' + name] = (typeof callback === 'function') ? callback : null;
	}

	addTimer(type, cb, time)
	{
		const id = ++this.__tid;
		this.__timers.set(id, [type, cb, 0.001*time, 0.0]);
		return id;
	}
	
	removeTimer(id)
	{
		this.__timers.delete(id);
	}
	
	static get title() {  return Application.__I.__title; }
	
	static set title(value) 
	{
		if(Application.__I.__title !== value)
		{
			Application.__I.__title = value;
			__f_app(0, value);
		}
	}
	
	static get state() {  return Application.__I.__state; }
	
	static set state(value) 
	{
		if(Application.__I.__state !== value)
		{
			Application.__I.__state = value;
			__f_app(1, value);
		}
	}
	
	static get size() {  return Application.__I.__state; }
	
	static set size(value) 
	{
		if(Array.isArray(value))
			__f_app(2, value[0], value[1]);
		else
			__f_app(2, value.width, value.height);
	}
	
	static get() { return Application.__I; }
	static quit() { __f_app(3); }
	static close() { __f_app(4); }
}

export function Color(r, g, b, a)
{
	const alpha = (a === undefined) ? 255 : a & 0xFF;
	return ((alpha & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | ((b & 0xFF));
};

export class Element
{
	constructor(props, type, _ptr)
	{
		this.__ = (_ptr) ? _ptr : __f_new(1, type, props);
		this.__onUpdate = null;
		if(props && props.parent)
			props.parent.add(this);
	}
	
	release()
	{
		__f_elem(2, this.__);
		delete this.__;
	}
	
	add(elem)
	{
		__f_elem(1, this.__, elem.__);
	}
	
	
	on(name, callback)
	{
		if(name === 'update')
		{
			if(this.__onUpdate !== null)
			{
				if(callback)
					Application.__I.processQueue[this.__onUpdate] = callback;
				else
				{
					Application.__I.__spares.push(this.__onUpdate);
					Application.__I.processQueue[this.__onUpdate] = null;
					this.__onUpdate = null;
				}
			}
			else if(callback)
			{
				if(Application.__I.__spares.length === 0)
					this.__onUpdate = Application.__I.processQueue.push(callback) - 1;
				else
				{
					this.__onUpdate = Application.__I.__spares.pop();
					Application.__I.processQueue[this.__onUpdate] = callback;
				}
			}
		}
	}
}