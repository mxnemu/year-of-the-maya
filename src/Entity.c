#include "Entity.h"
#include "Hitbox.h"
#include "Utils.h"
#include "Scene.h"

Entity* Entity_create(void* context, Scene* scene, AnimatedSprite* sprite) {
	Entity* this = malloc(sizeof(Entity));
	this->scene = scene;
	this->animatedSprite = sprite;
	this->destroyed = false;
	EntityPhysics_init(&this->physics, sprite ? sprite->sprite : NULL);
	this->update = emptyUpdate;
	this->draw = Entity_emptyDraw;
	this->destroy = emptyDestroy;
	this->context = context;
	this->hitboxes = List_create();
	this->speedMultiplier = 5;
	return this;
}

void Entity_destroy(Entity* this) {
	this->destroy(this->context);
	ListNode* it = this->hitboxes->first;
	while(it) {
		Hitbox_destroy((Hitbox*)it->data);
	}
	List_destroy(this->hitboxes);
	free(this);
}

bool Entity_collides(Entity* this, Entity* other) {
	if (SDL_Rect_touches(&this->physics.bounds, &other->physics.bounds)) {
		bool hitboxCollision = false;
		ListNode* it = this->hitboxes->first;
		while (it) {
			ListNode* jt = other->hitboxes->first;
			while (jt) {
				Hitbox* a = it->data;
				Hitbox* b = jt->data;
				if (Hitbox_collides(a, b)) {
					hitboxCollision = true; // TODO apply attacks / blocks and stuff
				}
			}
		}
		return hitboxCollision;
	}
	return false;
}


void EntityPhysics_init(EntityPhysics* this, Sprite* sprite) {
	if (sprite) {
		this->bounds.x = 0;
		this->bounds.y = 0;
		this->bounds.w = sprite->bounds.w * PHYSICS_SCALE;
		this->bounds.h = sprite->bounds.h * PHYSICS_SCALE;
	} else {
		SDL_Rect_init(&this->bounds);
	}
	this->dx = 0;
	this->dy = 0;
	this->groupMask = INT_MAX;
	this->groups = COLLISION_GROUP_DEFAULT;
}

void Entity_update(Entity* this, RawTime dt) {
	this->update(this->context, dt);

	if (this->animatedSprite) {
		AnimationProgress_update(&this->animatedSprite->progress, dt);
	}

	SDL_Rect newPosition = this->physics.bounds;
	newPosition.x += this->physics.dx;
	if (!Entity_wouldCollide(this, &newPosition)) {
		this->physics.bounds.x += this->physics.dx;
	}
	newPosition.x -= this->physics.dx;
	newPosition.y += this->physics.dy;
	if (!Entity_wouldCollide(this, &newPosition)) {
		this->physics.bounds.y += this->physics.dy;
	}
	this->physics.dx = this->physics.dy = 0;

	if (this->animatedSprite) {
		this->animatedSprite->sprite->bounds.x = this->physics.bounds.x / PHYSICS_SCALE;
		this->animatedSprite->sprite->bounds.y = this->physics.bounds.y / PHYSICS_SCALE;
	}
}

bool Entity_wouldCollide(Entity* this, SDL_Rect *rect) {
	// test scene
	if (rect->x < this->scene->walkableBounds.x ||
		rect->x + rect->w > this->scene->walkableBounds.x + this->scene->walkableBounds.w ||
		rect->y + rect->h < this->scene->walkableBounds.y ||
		rect->y + rect->h > this->scene->walkableBounds.y + this->scene->walkableBounds.h
		) {
		return true;
	}

	// test entities
	for (int i=0; i < this->scene->entities->usedElements; ++i) {
		Entity* it = this->scene->entities->elements[i];
		if (it != NULL) {

			for (int j=0; j < this->scene->entities->usedElements; ++j) {
				Entity* jt = this->scene->entities->elements[j];
				if (jt != NULL && jt != it && Entity_collides(it, jt)) {
					return true;
				}
			}
		}
	}
	return false;
}

void EntityPhysics_destroy(EntityPhysics* this) {
	free(this);
}

void Entity_emptyDraw(void* context, SDL_Renderer* surface, Camera* camera) {}
