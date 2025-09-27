CREATE TYPE event_type AS ENUM ('restaurant', 'travel', 'shared_house', 'shopping', 'entertainment', 'utilities', 'other');
CREATE TYPE event_status AS ENUM ('active', 'completed', 'cancelled');
CREATE TYPE participant_status AS ENUM ('active', 'inactive');
CREATE TYPE expense_split_type AS ENUM ('equal', 'percentage', 'custom');

CREATE TABLE events (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    creator_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    event_type event_type DEFAULT 'other',
    status event_status DEFAULT 'active',
    start_date TIMESTAMP WITH TIME ZONE,
    end_date TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT events_name_length CHECK (char_length(name) >= 1),
    CONSTRAINT events_date_order CHECK (end_date IS NULL OR start_date <= end_date)
);

CREATE TABLE expenses (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    event_id UUID NOT NULL REFERENCES events(id) ON DELETE CASCADE,
    payer_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    amount DECIMAL(10,2) NOT NULL,
    description VARCHAR(255) NOT NULL,
    expense_date TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    split_type expense_split_type DEFAULT 'equal',
    receipt_url TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT expenses_amount_positive CHECK (amount > 0),
    CONSTRAINT expenses_description_length CHECK (char_length(description) >= 1)
);

CREATE TABLE participants (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    event_id UUID NOT NULL REFERENCES events(id) ON DELETE CASCADE,
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    share_percentage DECIMAL(5,2),
    custom_amount DECIMAL(10,2),
    status participant_status DEFAULT 'active',
    joined_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT participants_unique_event_user UNIQUE (event_id, user_id),
    CONSTRAINT participants_share_percentage_valid CHECK (share_percentage IS NULL OR (share_percentage >= 0 AND share_percentage <= 100)),
    CONSTRAINT participants_custom_amount_positive CHECK (custom_amount IS NULL OR custom_amount >= 0)
);

CREATE INDEX idx_events_creator_id ON events(creator_id);
CREATE INDEX idx_events_status ON events(status);
CREATE INDEX idx_events_type ON events(event_type);
CREATE INDEX idx_events_dates ON events(start_date, end_date);

CREATE INDEX idx_expenses_event_id ON expenses(event_id);
CREATE INDEX idx_expenses_payer_id ON expenses(payer_id);
CREATE INDEX idx_expenses_date ON expenses(expense_date);

CREATE INDEX idx_participants_event_id ON participants(event_id);
CREATE INDEX idx_participants_user_id ON participants(user_id);
CREATE INDEX idx_participants_status ON participants(status);

CREATE TRIGGER update_events_updated_at BEFORE UPDATE ON events 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_expenses_updated_at BEFORE UPDATE ON expenses 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_participants_updated_at BEFORE UPDATE ON participants 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();