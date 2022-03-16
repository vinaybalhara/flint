#pragma once

namespace flint
{
	namespace javascript
	{
		static const char* const FlintModule = R"(export const ApplicationState={NORMAL:0,MINIMIZED:1,MAXIMIZED:2};class Event
{constructor(type){this.type=type;this.bubbles=!0;this.defaultPrevented=!1}
stopPropagation(){this.bubbles=!1}
preventDefault(){this.defaultPrevented=!0}}
class KeyboardEvent extends Event
{constructor(type,keyCode,key,flags){super(type);this.key=key;this.keyCode=keyCode;this.ctrlKey=!!(flags&2);this.shiftKey=!!(flags&4);this.altKey=!!(flags&8);this.metaKey=!!(flags&16);this.repeat=!!(flags&32);this.location=flags>>6}}
export class Application
{static Instance=null;constructor(args){Application.Instance=this;this.processQueue=[];this.__spares=[];this.__timers=new Map();this.__events={};this.__tid=0;const result=__f_init(args);this.__title=result[2];this.__state=result[3]}
update(delta){if(this.onupdate)
this.onupdate(delta);for(let i=0;i!==this.processQueue.length;++i){const callback=this.processQueue[i];if(callback)
callback(delta)}
for(let[id,timer]of this.__timers){timer[3]+=delta;if(timer[3]>=timer[2]){timer[1]();timer[3]%=timer[2];if(timer[0]===1)
this.__timers.delete(id)}}}
dispatch(event){const own=this['on'+event.type];if(own){own(event);if(!event.bubbles)
return}
const listeners=this.__events[event.type];if(listeners){for(let listener of listeners){listener(event);if(!event.bubbles)
return}}
return!event.defaultPrevented}
fireEvent(type,a,b,c){if(type===1)
return this.dispatch(new KeyboardEvent('keyup',a,b,c));else if(type==2)
return this.dispatch(new KeyboardEvent('keydown',a,b,c));return!0}
static addEventListener(name,callback){if(typeof callback==='function'){let target=Application.Instance.__events[name];if(target===undefined)
target=Application.Instance.__events[name]=new Set();target.add(callback)}}
static removeEventListener(name,callback){if(typeof callback==='function'){let target=Application.Instance.__events[name];if(target)
target.delete(callback)}}
on(name,callback){this['on'+name]=(typeof callback==='function')?callback:null}
addTimer(type,cb,time){const id=++this.__tid;this.__timers.set(id,[type,cb,0.001*time,0.0]);return id}
removeTimer(id){this.__timers.delete(id)}
static get title(){return Application.Instance.__title}
static set title(value){if(Application.Instance.__title!==value){Application.Instance.__title=value;__f_app(0,value)}}
static get state(){return Application.Instance.__state}
static set state(value){if(Application.Instance.__state!==value){Application.Instance.__state=value;__f_app(1,value)}}
static get size(){return Application.Instance.__state}
static set size(value){if(Array.isArray(value))
__f_app(2,value[0],value[1]);else __f_app(2,value.width,value.height)}
static get(){return Application.Instance}
static quit(){__f_exit()}
static close(){__f_exit(1)}}
export class Container
{constructor(){this.__=__f_new(1);__f_set(this.__);this.__onUpdate=null}
on(name,callback){if(name==='update'){if(this.__onUpdate!==null){if(callback)
Application.Instance.processQueue[this.__onUpdate]=callback;else{Application.Instance.__spares.push(this.__onUpdate);Application.Instance.processQueue[this.__onUpdate]=null;this.__onUpdate=null}}else if(callback){if(Application.Instance.__spares.length===0)
this.__onUpdate=Application.Instance.processQueue.push(callback)-1;else{this.__onUpdate=Application.Instance.__spares.pop();Application.Instance.processQueue[this.__onUpdate]=callback}}}}})";

	}
}